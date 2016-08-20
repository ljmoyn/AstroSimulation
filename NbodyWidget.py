from PyQt4 import QtCore, QtGui, QtOpenGL
from OpenGL import GL, GLU, GLUT

import copy

class NbodyWidget(QtOpenGL.QGLWidget):
    cameraAzimuthChanged = QtCore.pyqtSignal(int)
    cameraElevationChanged = QtCore.pyqtSignal(int)
    
    def __init__(self, simulation, isPaused, isRealtime, viewFocusIndex, precomputeTimestep, precomputeTotalTime, playbackSpeed, playbackIndex,  parent = None):
        super(NbodyWidget, self).__init__(parent)
        self.objectPoints = 0
        self.objectPaths = 0
        
        self.historyData = []
        self.precomputedData = []
                
        self.viewFocusIndex = viewFocusIndex
        self.isPaused = isPaused
        
        self.isRealtime = isRealtime
        self.precomputeTimestep = precomputeTimestep # years
        self.precomputeTotalTime = precomputeTotalTime # years
        self.playbackSpeed = playbackSpeed #timesteps per (real) second
        self.playbackIndex = playbackIndex #where we are in the playback
        
        #set a 1:1 aspect ratio for the widget. See heightForWidth() function
# Note, worked on ubuntu, doesn't work on windows 10        
#         sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Preferred, QtGui.QSizePolicy.Preferred)
#         sizePolicy.setHeightForWidth(True)
#         self.setSizePolicy(sizePolicy)        
        self.setFixedSize(QtCore.QSize(930,930))
        self.cameraAzimuth = 0
        self.cameraElevation = 90
        self.cameraTranslation = [0,0]
                
        self.xRange = [-1000,1000]
        self.yRange = [-1000,1000]
        self.zRange = [-10000,10000]        
        
        self.simulation = simulation

        self.lastPos = QtCore.QPoint()
        
        self.backgroundColor = QtGui.QColor.fromCmykF(0.95, 0.95, 0.95, 0.95)
        
        self.simulationSpeed = .5/365
                
        self.prevT = GLUT.glutGet(GLUT.GLUT_ELAPSED_TIME)
        timer = QtCore.QTimer(self)
        timer.timeout.connect(self.animate)
        timer.start(50)
        
        #for some reason, it is necessary to step the simulation before changes in object stats 
        #are actually recognized. Without this, if you make any changes before starting the 
        #simulation for the first time, the changes will be overwritten in the first step.
        #honestly not sure why. It's really weird...
        self.simulation.step(0)        

        
    #used in QSizePolicy to determine the preferred aspect ratio for the widget. 
    #I want to keep it square, so I just return width
# Note: worked on ubuntu, doesn't work on windows 10        
#     def heightForWidth(self,width):
#         return width        
    
    #resize viewport whenever the widget is resized. Prevents unwanted clipping
    def resizeGL(self,width,height):    
        GL.glViewport(0,0,width,width)
    
    def animate(self):
        t = GLUT.glutGet(GLUT.GLUT_ELAPSED_TIME)        
        if not self.isPaused:
            realElapsedTime = (t - self.prevT)/1000.0 #(in seconds)     
            self.update(realElapsedTime)
            
            self.parent().parent().timerRefresh()
            
            
        self.prevT = t
    
    def clearHistory(self):
        self.historyData = []
    
    def update(self,realElapsedTime):
        #simulation time is in years, real time is in seconds
        simulationElapsedTime = self.simulationSpeed * realElapsedTime
        
        if self.isRealtime:
            self.simulation.step(simulationElapsedTime)
            if len(self.historyData) < 1000:
                self.historyData.append(copy.deepcopy(self.simulation))        
            else:
                self.historyData.append(copy.deepcopy(self.simulation))
                #todo: consider using deque object for historyData, if efficiency becomes an issue
                self.historyData.pop(0)
        elif ((self.playbackSpeed >= 0 and self.playbackIndex + self.playbackSpeed < len(self.precomputedData)) or 
             (self.playbackSpeed < 0 and self.playbackIndex + self.playbackSpeed > 0)):
            self.playbackIndex += self.playbackSpeed
            self.simulation = self.precomputedData[self.playbackIndex]
            
            playbackSlider = self.parent().parent().playbackSlider
            playbackSlider.blockSignals(True)
            playbackSlider.setValue(self.playbackIndex)
            playbackSlider.blockSignals(False)
        elif self.playbackSpeed >= 0:
            self.playbackIndex = 0
        else:
            self.playbackIndex = len(self.precomputedData)
                            
        self.objectPoints = self.makePoints()
        self.objectPaths = self.makePaths()     
        
        self.simulation.time += simulationElapsedTime
        self.updateGL()
        
    def refreshPointsAndPaths(self):
        self.objectPoints = self.makePoints()
        self.objectPaths = self.makePaths()     
        
        self.updateGL()            

    def setCameraAzimuth(self, angle):
        if angle != self.cameraAzimuth:
            self.cameraAzimuth = angle
            self.cameraAzimuthChanged.emit(angle)
            self.updateGL()

    def setCameraElevation(self, angle):
        if angle != self.cameraElevation:
            self.cameraElevation = angle
            self.cameraElevationChanged.emit(angle)
            self.updateGL()
    
    #sets the camera to focus on a given position. x and y are in simulation coordinates
    def setCameraPosition(self, x, y):
        average = (self.xRange[1] - self.xRange[0]) / 2
        self.xRange[0] = -average;
        self.xRange[1] = average;
        
        average = (self.yRange[1] - self.yRange[0]) / 2
        self.yRange[0] = -average;
        self.yRange[1] = average;
                
        GL.glMatrixMode(GL.GL_PROJECTION)
        GL.glLoadIdentity()         
        GL.glOrtho(self.xRange[0], self.xRange[1], self.yRange[0], self.yRange[1], self.zRange[0], self.zRange[1])
        
        GL.glMatrixMode(GL.GL_MODELVIEW)
        self.updateGL()  
    
    #moves the camera a given amount. dx and dy are in pixels.
    def setCameraTranslation(self,dx,dy):
        viewport = GL.glGetIntegerv(GL.GL_VIEWPORT)
        scaleX = (self.xRange[1] - self.xRange[0])/viewport[2]
        scaleY = (self.yRange[1] - self.yRange[0])/viewport[3]       
        
        dx = scaleX * dx
        dy = scaleY * dy        
        
        self.xRange[0] = self.xRange[0] - dx;
        self.xRange[1] = self.xRange[1] - dx;

        self.yRange[0] = self.yRange[0] + dy;
        self.yRange[1] = self.yRange[1] + dy;
        
        GL.glMatrixMode(GL.GL_PROJECTION)
        GL.glLoadIdentity()         
        GL.glOrtho(self.xRange[0], self.xRange[1], self.yRange[0], self.yRange[1], self.zRange[0], self.zRange[1])
        
        GL.glMatrixMode(GL.GL_MODELVIEW)
        self.updateGL()        

    def initializeGL(self):

        self.qglClearColor(self.backgroundColor.dark())
        self.objectPoints = self.makePoints()
        self.objectPaths = self.makePaths()

        GL.glShadeModel(GL.GL_FLAT)
        GL.glEnable(GL.GL_DEPTH_TEST)
        GL.glEnable(GL.GL_CULL_FACE)

        GL.glMatrixMode(GL.GL_PROJECTION)
        GL.glLoadIdentity()      
               
        GL.glOrtho(self.xRange[0], self.xRange[1], self.yRange[0], self.yRange[1], self.zRange[0], self.zRange[1])
        
        GL.glMatrixMode(GL.GL_MODELVIEW)

    def paintGL(self):
        GL.glClear(GL.GL_COLOR_BUFFER_BIT | GL.GL_DEPTH_BUFFER_BIT)
        GL.glLoadIdentity()
        
        # note: it's glRotate + d for double, not past tense
        #camera defaults to pointing in the -z direction (into the screen).
        #since the ecliptic is set up on the xy plane, the camera defaults to looking down on the ecliptic (90 deg elevation)
        #so if elevation is 90, we want want 0 camera rotation, and so on...
        GL.glRotated(self.cameraElevation-90, 1.0, 0.0, 0.0);
        GL.glRotated(self.cameraAzimuth, 0.0, 0.0, 1.0);
    
        GL.glCallList(self.objectPoints)
        GL.glCallList(self.objectPaths)


    def mousePressEvent(self, event):
        self.lastPos = event.pos() 

    def GetOGLPos(self, x, y):     
     
        viewport = GL.glGetIntegerv(GL.GL_VIEWPORT)
        winX = x;
        winY = viewport[3] - y;
        winZ = GL.glReadPixels(winX, winY, 1, 1, GL.GL_DEPTH_COMPONENT, GL.GL_FLOAT)

        (posX,posY,posZ) = GLU.gluUnProject( winX, winY, winZ)
     
        return (posX, posY, posZ);

    def wheelEvent(self, event):
        GL.glMatrixMode(GL.GL_MODELVIEW)
        GL.glLoadIdentity()    
    
        (posX,posY,posZ) = self.GetOGLPos(event.x(),event.y()) 

        direction = event.delta()        
        if direction < 0:
            zoomFactor = 1.1
        else:
            zoomFactor = 1/1.1
            
        leftSegment = (posX - self.xRange[0]) * zoomFactor
        self.xRange[0] = posX - leftSegment
        
        rightSegment = (self.xRange[1] - posX) * zoomFactor
        self.xRange[1] = posX + rightSegment
        
        bottomSegment = (posY - self.yRange[0]) * zoomFactor
        self.yRange[0] = posY - bottomSegment

        topSegment = (self.yRange[1] - posY) * zoomFactor
        self.yRange[1] = posY + topSegment        
        
        GL.glMatrixMode(GL.GL_PROJECTION)
        GL.glLoadIdentity()         
        GL.glOrtho(self.xRange[0], self.xRange[1], self.yRange[0], self.yRange[1], self.zRange[0], self.zRange[1])
        
        GL.glMatrixMode(GL.GL_MODELVIEW)
        self.updateGL()

    def mouseMoveEvent(self, event):
        dx = event.x() - self.lastPos.x()
        dy = event.y() - self.lastPos.y()

        if event.buttons() & QtCore.Qt.LeftButton:
            self.setCameraTranslation(dx,dy)
        if event.buttons() & QtCore.Qt.RightButton:
            self.setCameraAzimuth(self.cameraAzimuth + dx)
            self.setCameraElevation(self.cameraElevation + dy)

        self.lastPos = event.pos()

    def makePoints(self):  
        genList = GL.glGenLists(1)
        GL.glNewList(genList, GL.GL_COMPILE)
        GL.glPointSize(3)

        GL.glBegin(GL.GL_POINTS)

        for i,currentObject in enumerate(self.simulation.objects):
            settings = self.simulation.objectSettings[i]             
            x = currentObject.position[0]
            y = currentObject.position[1]
            z = currentObject.position[2]
            GL.glColor3f(settings.color[0],settings.color[1],settings.color[2])
            
            if self.viewFocusIndex > -1:
                focus = self.simulation.objects[self.viewFocusIndex]
                
                x -= focus.position[0]
                y -= focus.position[1]
                z -= focus.position[2]
            
#             never want the view to be cut off in the (GL) z direction (into/out of the screen), so automatically expand the limits when necessary
#             if abs(x) > self.zRange[1] or abs(y) > self.zRange[1] or abs(z) > self.zRange[1]:      
#                 self.zRange[1] = max(abs(x),abs(y),abs(z))
#                 print(self.zRange)
            
            GL.glVertex3d(x, y, z)

        GL.glEnd()
        
        GL.glEndList()         
        
        return genList
        
    def makePaths(self):
        numObjects = len(self.simulation.objects)
        genList = GL.glGenLists(numObjects)
        GL.glNewList(genList, GL.GL_COMPILE)

        pathDataSource = self.historyData
        if not self.isRealtime:
            pathDataSource = self.precomputedData[0:self.playbackIndex + 1]

        for i in range(numObjects):
            settings = self.simulation.objectSettings[i] 
            if settings.showHistory is True:
                GL.glBegin(GL.GL_LINE_STRIP)              
                GL.glColor3f(settings.color[0],settings.color[1],settings.color[2])
                
                for step in pathDataSource:
                    stepObjects = step.objects            
                    x = stepObjects[i].position[0]
                    y = stepObjects[i].position[1]
                    z = stepObjects[i].position[2]
                    
                    if self.viewFocusIndex > -1:
                        focus = self.simulation.objects[self.viewFocusIndex]
                        
                        x -= focus.position[0]
                        y -= focus.position[1]
                        z -= focus.position[2]                      
                    
                    GL.glVertex3d(x, y, z)
                GL.glEnd()

        GL.glEndList()

        return genList
