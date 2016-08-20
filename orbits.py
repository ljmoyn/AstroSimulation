import numpy

from scipy.spatial import distance
from scipy import integrate
import copy
from _overlapped import NULL


#units
#distance in billions of meters
#time in years
G = 9.94519*10**14 * 6.67408 * 10**-11 / (10**9)**3

class OrbitalSimulation:
    def __init__(self,objects,objectSettings,time):
        self.objects = objects
        self.objectSettings = objectSettings
        
        #1d vector containing position/velocity data, is used to store the output from integrate.odeint
        #[x1,y1,z1,Vx1,Vy1,Vz1,x2,y2,z2...]
        #technically redundant, since the data is the same as in self.objects, but kept around for efficiency
        #storing it saves another call to GetFlattenedObjects in the step function 
        self.state = self.GetFlattenedObjects()
        self.time = time;

    def orbit(self,y,t):
        global G
        
        objects = []
        i=0;
        j=0;
        while i+6 <= len(y):
            position = y[i:i+3]
            velocity = y[i+3:i+6]
            objects.append(OrbitalObject(self.objects[j].name,self.objects[j].mass,position,velocity))
            i+=6
            j+=1
                
        dydt = []
        for obj1 in objects:
            newVelocity = obj1.velocity
            newAcceleration = [0,0,0]
            for obj2 in objects:
                if obj1 != obj2:
                    r = distance.pdist([obj1.position,obj2.position])
                    newAcceleration += -G * obj2.mass * (obj1.position - obj2.position) / (r**3)
            dydt.extend(newVelocity)
            dydt.extend(newAcceleration)
            
        return dydt    

    def precomputePositions(self, timestep, totalTime, progress):
        simCopy = copy.deepcopy(self)
        
        results = [copy.deepcopy(simCopy)]
        i = 1
        while i < round(totalTime / timestep):
            simCopy.step(timestep)
            results.append(copy.deepcopy(simCopy))       
             
            i+=1

        return results
        
# Kind of nervous about all the memory this is going to take up. If it becomes necessary, use the below instead, to store just positions/velocities        
#         results = [{
#             "positions": [],
#             "velocities": []
#         }]
#         for obj in simCopy.objects:
#             results[0]["positions"].append(copy.deepcopy(obj.position))
#             results[0]["velocities"].append(copy.deepcopy(obj.velocity))
#         
#         i = 1
#         while i < round(totalTime / timestep):
#             simCopy.step(timestep)
#             
#             results.append({
#                 "positions": [],
#                 "velocities": []
#             })
#             for obj in simCopy.objects:
#                 results[i]["positions"].append(copy.deepcopy(obj.position))
#                 results[i]["velocities"].append(copy.deepcopy(obj.velocity))            
#             
#             i+=1

    #convert between the intuitive list of 'objects' and the 1d pos/vel array used by odeint 
    def GetFlattenedObjects(self,forward = True):
        output=[]    
    
        if forward is True:
            for obj in self.objects:
                output.extend(obj.position)
                output.extend(obj.velocity)                    
        else: 
            i=0;
            j=0;
            while i+6 <= len(self.state):
                position = self.state[i:i+3]
                velocity = self.state[i+3:i+6]
                output.append(OrbitalObject(self.objects[j].name,self.objects[j].mass,position,velocity))
                i+=6
                j+=1
                
        return output
        
    def PlotPosition(self):    
        x=[]
        y=[]
        z=[]
        for obj in self.objects:
            x.append(obj.position[0])
            y.append(obj.position[1])
            z.append(obj.position[2])
        return (x,y,z)           
            
    def step(self,dt):
        if dt != 0:        
            #self.velocityVerlet(dt)
            self.rungeKutta4(dt)
            #consider getting rid of self.state, making it local to this function.
            #downside is I'd have to call GetFlattenedObjects for the odeint parameter
            #self.state = integrate.odeint(self.orbit,self.state,[0, dt])[1]
            #self.objects = self.GetFlattenedObjects(forward = False)
            self.time += dt
    
    #returns 2d vector containing the accelerations of all the objects
    def getGravAccelerations(self, objects = None):
        if objects is None:
            objects = self.objects
        
        accelerations = []
        for obj1 in objects:
            acceleration = [0,0,0]
            for obj2 in objects:
                if obj1 != obj2:
                    rMag = distance.pdist([obj1.position,obj2.position])
                    rUnit = (obj1.position - obj2.position) / rMag  
                    acceleration += -G * obj2.mass * rUnit / (rMag**2)
    
            accelerations.append(acceleration)
            
        return accelerations
    
    #source: http://physics.ucsc.edu/~peter/242/leapfrog.pdf
    def velocityVerlet(self,dt):
        accelerations = self.getGravAccelerations()
        #get velocities of objects at +1/2 timestep. Eq. 13a
        for i,currentObject in enumerate(self.objects):
            currentObject.velocity += .5 * dt * accelerations[i]
         
        #get position at +1 timestep, using velocity at half timestep
        for i,currentObject in enumerate(self.objects):
            currentObject.position += dt * currentObject.velocity
                         
        accelerations = self.getGravAccelerations()
         
        #get velocity at +1 timestep, using velocity at +1/2 timestep, and accelerations based on position at next timestep
        for i,currentObject in enumerate(self.objects):
            currentObject.velocity += .5 * dt * accelerations[i]   
            
    def rungeKutta4(self, dt):
        
        #k1: current velocity/acceleration of the objects
        #note that the parameters are 2d vectors, containing data for all objects
        k1 = {
            "velocities" :[],
            "accelerations": self.getGravAccelerations(copy.deepcopy(self.objects))
        }
        for obj in self.objects:
            k1["velocities"].append(obj.velocity)
        
        #k2: velocity/acceleration at +1/2 timestep
        #computed assuming the current velocity/acceleration are accurate for the full period (t ---> t+1/2)
        k2 = {
            "velocities" :[],
            "accelerations": []
        }
        k2Objects = copy.deepcopy(self.objects)
        for i,obj in enumerate(k2Objects):
            obj.position += .5 * dt * k1["velocities"][i]
            obj.velocity += .5 * dt * k1["accelerations"][i]
            k2["velocities"].append(obj.velocity)
        
        k2["accelerations"] = self.getGravAccelerations(k2Objects)
        
        #k3: velocity/acceleration at +1/2 timestep
        #computed assuming the velocity/acceleration for the end of the period (k2) are accurate for the full period (t ---> t+1/2)
        k3 = {
            "velocities" :[],
            "accelerations": []
        }
        k3Objects = copy.deepcopy(self.objects)
        for i,obj in enumerate(k3Objects):
            obj.position += .5 * dt * k2["velocities"][i]
            obj.velocity += .5 * dt * k2["accelerations"][i]
            k3["velocities"].append(obj.velocity)
        
        k3["accelerations"] = self.getGravAccelerations(k3Objects)
        
        #k4: velocity/acceleration at +1 timestep
        #computed assuming the velocity/acceleration for the midpoint (k3) are accurate for the full period (t ---> t+1)
        k4 = {
            "velocities" :[],
            "accelerations": []
        }
        k4Objects = copy.deepcopy(self.objects)
        for i,obj in enumerate(k4Objects):
            obj.position += dt * k3["velocities"][i]
            obj.velocity += dt * k3["accelerations"][i]
            k4["velocities"].append(obj.velocity)
        
        k4["accelerations"] = self.getGravAccelerations(k4Objects)
        
        #finally, for a better estimate of the position/velocity at t +1 timestep, average the values from the 4 increments, with greater weight on the midpoint values
        for i,obj in enumerate(self.objects):
            obj.position += dt * (k1["velocities"][i] + 2*k2["velocities"][i] + 2*k3["velocities"][i] + k4["velocities"][i]) / 6
            obj.velocity += dt * (k1["accelerations"][i] + 2*k2["accelerations"][i] + 2*k3["accelerations"][i] + k4["accelerations"][i]) / 6            
        
class Ellipse:
    def __init__(self, semimajorAxis,semiminorAxis = None, eccentricity=None):
        self.a = semimajorAxis
        if eccentricity is None and semiminorAxis is not None:
            self.b = semiminorAxis
            self.e = numpy.sqrt(1-self.b**2/self.a**2)
        elif eccentricity is not None:
            self.e = eccentricity
            self.b = self.a * numpy.sqrt(1 - self.e**2)
        else:
            self.e = 0
            self.b = self.a 

#note: position and velocity vectors are expected to be numpy arrays, rather than regular python lists
class OrbitalObject:
    def __init__(self,Name,Mass,Position,Velocity = numpy.array([0,0,0]),CenterObject = None, OrbitEllipse = None):
        self.name = Name
        self.mass = Mass
        self.position = Position
        if CenterObject is None or OrbitEllipse is None:
            self.velocity = Velocity
        else:
            r = distance.pdist([CenterObject.position,self.position])
            speed = numpy.sqrt(G*CenterObject.mass*(2/r-1/OrbitEllipse.a)) + CenterObject.GetSpeed()
            
            #credit to this http://orbiter-forum.com/showthread.php?t=24457
            k = r / OrbitEllipse.a
            phi = (2-2*OrbitEllipse.e**2)/(k*(2-k)) - 1
            if phi > 1:
                phi = 1;
                
            alpha = numpy.arccos(phi)                  
            cosAlpha = numpy.cos((numpy.pi - alpha) / 2)
            sinAlpha = numpy.sin((numpy.pi - alpha) / 2)
            rotMatrix = [[cosAlpha, -sinAlpha, 0],[sinAlpha,cosAlpha,0],[0,0,1]]
            direction = (numpy.array(self.position) - numpy.array(CenterObject.position)) / r
            direction = numpy.dot(rotMatrix,direction)
            self.velocity = direction*speed

    def GetSpeed(self):
        return numpy.sqrt(self.velocity[0]**2 + self.velocity[1]**2 + self.velocity[2]**2)
        
    def SetPosition(self, newPosition, index):
        self.position[index] = newPosition
        
    def SetVelocity(self, newVelocity, index):
        self.velocity[index] = newVelocity        
        
    def SetMass(self, newMass):
        self.mass = newMass