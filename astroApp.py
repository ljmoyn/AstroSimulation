import sys
import re

import numpy
import copy

from orbits import OrbitalObject, OrbitalSimulation, Ellipse
from NbodyWidget import NbodyWidget
from ObjectSettings import ObjectSettingsPopup, ObjectSettings
from ScientificDoubleSpinBox import ScientificDoubleSpinBox, valid_float_string

from xml.etree import ElementTree
from xml.dom import minidom


from PyQt4 import QtCore, QtGui, uic

MainWindow, MainWindowUi = uic.loadUiType('design.ui')
     
class Main(MainWindow, MainWindowUi):
    def __init__(self,xml):
        super(Main, self).__init__()
        self.setupUi(self)
        
        self.currentPage = 0
        
        self.initButtons(xml)        
        self.initObjectControls()        
        
        self.refreshAccordion()        
        self.refreshObjectValues()
        self.objectsSetEnabled(self.glWidget.isPaused)        
        
        #fixed size because rapidly resizing causes an unexplained crash on windows 10. was not a problem in my ubuntu environment
        self.setFixedSize(1500,1000)      
        self.show()
        
#        self.showFullScreen()     

    def initButtons(self, xml):
        #add opengl widget to layout
        objects,settings,time = self.fromXml(xml, True)
        initialSimulation = OrbitalSimulation(objects,settings,time)
        
        self.glWidget = NbodyWidget(initialSimulation,
                                    True, #isPaused,
                                    True, #isRealtime
                                    -1, #viewFocusIndex
                                    self.precomputeTimestep.value() / 365, #days in the ui, years under the hood
                                    self.precomputeTotalTime.value(),
                                    self.playbackSpeed.value(),
                                    0, #playbackIndex
                                    parent=self)
        self.plotCanvas.addWidget(self.glWidget)
        

        self.saveButton.clicked.connect(self.save_callback)
        self.loadButton.clicked.connect(self.load_callback)

        #initialize play/pause button
        self.pauseIcon = QtGui.QIcon()
        self.pauseIcon.addPixmap(QtGui.QPixmap("resources/icons/icon-pause.png"))
        self.playIcon = QtGui.QIcon()
        self.playIcon.addPixmap(QtGui.QPixmap("resources/icons/icon-play.png"))
        self.pauseButton.setIcon(self.playIcon)
        self.pauseButton.setIconSize(QtCore.QSize(32,32))        
        self.pauseButton.clicked.connect(self.pause_callback)
        
        self.playbackPauseButton.setIcon(self.playIcon)
        self.playbackPauseButton.setIconSize(QtCore.QSize(32,32))        
        self.playbackPauseButton.clicked.connect(self.pause_callback)        
        
        self.simSpeedValue.setValue(self.glWidget.simulationSpeed * 365)
        self.simSpeedValue.valueChanged.connect(self.simSpeedValue_callback)
        
        self.computeMode.currentChanged.connect(self.computeMode_callback)
        
        self.precomputeTimestep.valueChanged.connect(self.precomputeTimestep_callback)
        self.precomputeTotalTime.valueChanged.connect(self.precomputeTotalTime_callback)
        
        self.playbackSlider.valueChanged.connect(self.playbackSlider_callback)
        self.playbackSlider.setTracking(False)
        
        self.playbackSpeed.valueChanged.connect(self.playbackSpeed_callback)        
        self.computeButton.clicked.connect(self.computeButton_callback)

        self.edgeView.clicked.connect(self.edgeView_callback)        
        self.topView.clicked.connect(self.topView_callback)        
        
        self.nextPage.clicked.connect(self.changePageFactory(1))
        self.prevPage.clicked.connect(self.changePageFactory(-1))
        
        self.updateViewFocus()
        
        self.viewFocus.currentIndexChanged.connect(self.viewFocus_callback)
        
        #update current object values when user changhes what they're looking at
        self.ObjectAccordion.currentChanged.connect(self.refreshObjectValues)  
    
    def initObjectControls(self):
                
        ObjectDetailsLayouts = [self.ObjectDetailsLayout_1,
                                self.ObjectDetailsLayout_2,
                                self.ObjectDetailsLayout_3,
                                self.ObjectDetailsLayout_4,
                                self.ObjectDetailsLayout_5]
        
        ObjectSettingsLayouts = [self.ObjectSettingsLayout_1,
                                self.ObjectSettingsLayout_2,
                                self.ObjectSettingsLayout_3,
                                self.ObjectSettingsLayout_4,
                                self.ObjectSettingsLayout_5]        
        
        self.ObjectControls = {}
        for i in range(len(ObjectDetailsLayouts)):
            settings = QtGui.QPushButton("Settings")
            settings.setFixedWidth(80)
            
            mass = ScientificDoubleSpinBox() 
            x = ScientificDoubleSpinBox()   
            y = ScientificDoubleSpinBox()                                 
            z = ScientificDoubleSpinBox()                                 
            Vx = ScientificDoubleSpinBox()                                 
            Vy = ScientificDoubleSpinBox()                                 
            Vz = ScientificDoubleSpinBox()

            #initialize the dict of controls for the ith layout
            objectControls = {
            "Mass_{0}".format(i) : mass,
            "X_{0}".format(i) : x,
            "Y_{0}".format(i) : y,
            "Z_{0}".format(i) : z,
            "Vx_{0}".format(i) : Vx,
            "Vy_{0}".format(i) : Vy,
            "Vz_{0}".format(i) : Vz
            }
            
            #wire the editable controls
            for key,control in objectControls.items():
                control.valueChanged.connect(self.setObjectValueFactory(key))          
            
            #wire settings button and add it to the current layout dict
            settings.clicked.connect(self.openObjectSettingsFactory(i))
            objectControls.update({"ObjectSettings_{0}".format(i) : settings})
            
            #add controls to the gui
            ObjectSettingsLayouts[i].insertWidget(0,settings)
            ObjectDetailsLayouts[i].addRow("Mass (Kg):",mass)
            
            positionLayout = QtGui.QHBoxLayout()
            positionLayout.addWidget(x)
            positionLayout.addWidget(y)
            positionLayout.addWidget(z)
            ObjectDetailsLayouts[i].addRow("Position (Gm):",positionLayout)
            
            velocityLayout = QtGui.QHBoxLayout()
            velocityLayout.addWidget(Vx)
            velocityLayout.addWidget(Vy)
            velocityLayout.addWidget(Vz)
            ObjectDetailsLayouts[i].addRow("Velocity (Gm / year):",velocityLayout)                
             
            #add the current layout controls to the dict of all object controls    
            self.ObjectControls.update(objectControls)                  
            
    ###########################################################################################
    ### Callbacks                                                                           ###
    ### Directly connected to a button click or user input                                  ###
    ###########################################################################################
    def pause_callback(self):
        if self.glWidget.isPaused:
            self.pauseButton.setIcon(self.pauseIcon)
            self.playbackPauseButton.setIcon(self.pauseIcon)            
        else:
            self.pauseButton.setIcon(self.playIcon)
            self.playbackPauseButton.setIcon(self.playIcon)
            
        self.glWidget.isPaused = not self.glWidget.isPaused
        self.objectsSetEnabled(self.glWidget.isPaused)  
        
    def simSpeedValue_callback(self, newSpeed):
        self.glWidget.simulationSpeed = newSpeed / 365 
        
    def precomputeTimestep_callback(self, newTimestep):
        self.glWidget.precomputeTimestep = newTimestep / 365 
        
    def precomputeTotalTime_callback(self, newTotal):
        self.glWidget.precomputeTotalTime = newTotal

    def playbackSpeed_callback(self, newSpeed):
        self.glWidget.playbackSpeed = newSpeed   
        
    def computeButton_callback(self):            
        simCopy = copy.deepcopy(self.glWidget.simulation)
        
        totalTimesteps = round(self.glWidget.precomputeTotalTime / self.glWidget.precomputeTimestep)
        progress = QtGui.QProgressDialog("Computing 0 / {0} timesteps".format(totalTimesteps), "Cancel", 0, totalTimesteps);
        progress.setModal(True);        
        
        self.glWidget.precomputedData = [copy.deepcopy(simCopy)]
        i = 1
        while i < totalTimesteps:
            progress.setValue(i)
            progress.setLabelText("Computing {0} / {1} timesteps".format(i, totalTimesteps))
            simCopy.step(self.glWidget.precomputeTimestep)
            self.glWidget.precomputedData.append(copy.deepcopy(simCopy))       
            
            if progress.wasCanceled():
                self.glWidget.precomputedData = []
                return
             
            i+=1
        
        self.glWidget.playbackIndex = 0
        self.refreshPrecomputeControls()
        
    def topView_callback(self):
        self.glWidget.setCameraAzimuth(0)
        self.glWidget.setCameraElevation(90)

    def edgeView_callback(self):
        self.glWidget.setCameraAzimuth(0)
        self.glWidget.setCameraElevation(0)
        
    def viewFocus_callback(self, index):
        #want -1 for no focus
        self.glWidget.viewFocusIndex = index - 1
        
        if index > 0:
            objectPosition = self.glWidget.simulation.objects[index-1].position
            self.glWidget.setCameraPosition(objectPosition[0],objectPosition[1])
            if self.glWidget.isPaused:
                self.glWidget.refreshPointsAndPaths()
        
    def save_callback(self):
        filename = QtGui.QFileDialog.getSaveFileName(self, "Save File","saves/","XML files (*.xml)")     
        if filename != "":
            xml = self.toXml()
            with open(filename, "w") as file:
                file.write(xml) 
                       
    def load_callback(self):
        filename = QtGui.QFileDialog.getOpenFileName(self, "Load File","saves/","XML files (*.xml)")     
        if filename != "":
            with open(filename, "r") as file:
                xml = file.read() 
                self.fromXml(xml)
                
    def computeMode_callback(self, index):
        self.glWidget.historyData = []
        if index == 0:
            self.glWidget.isRealtime = True
        else:
            self.glWidget.isRealtime = False
            self.refreshPrecomputeControls()
            
    def playbackSlider_callback(self, newIndex):
        self.glWidget.playbackIndex = newIndex
        self.glWidget.simulation = self.glWidget.precomputedData[newIndex]

        self.glWidget.refreshPointsAndPaths()

    ###########################################################################################
    ### Factories                                                                           ###
    ### Sometimes use a 'factory' function instead of a callback because when you connect a ###
    ### button to its callback you can't pass a parameter directly                          ###
    ###########################################################################################
    
    def changePageFactory(self, increment):
        def changePage():
            self.currentPage +=increment
            
            numObjects = len(self.glWidget.simulation.objects)
            remainder = numObjects % 5
            totalPages = (numObjects - remainder) / 5
            isLastPage = totalPages == self.currentPage
                          
            if isLastPage:
                self.nextPage.setEnabled(False)
            else:
                self.nextPage.setEnabled(True)
    
            if self.currentPage is 0:
                self.prevPage.setEnabled(False)
            else:
                self.prevPage.setEnabled(True)
                
            self.refreshAccordion()
            self.refreshObjectValues()
                
        return changePage 
    
    def openObjectSettingsFactory(self, controlIndex):
        def openObjectSettings():
            objectIndex = 5*self.currentPage + controlIndex
            name = self.glWidget.simulation.objects[objectIndex].name
            objectSettingsPopup = ObjectSettingsPopup(name,self.glWidget.simulation.objectSettings[objectIndex])
            
            if objectSettingsPopup.exec_() and objectSettingsPopup.result() == QtGui.QDialog.Accepted:
                self.glWidget.simulation.objectSettings[objectIndex] = objectSettingsPopup.getNewSettings()
                self.refreshAccordion()
                self.glWidget.update(0)
        
        return openObjectSettings
         
    def setObjectValueFactory(self,key):
        def setObjectValue(newValue):
            if self.ObjectControls[key].isEnabled():
                numObjects = len(self.glWidget.simulation.objects)        
                for i in range(5):
                    objectIndex = 5*self.currentPage + i
                    if self.ObjectAccordion.currentIndex() is i and objectIndex < numObjects:
                        positionKeys = ["X_{0}".format(i),"Y_{0}".format(i), "Z_{0}".format(i)]
                        velocityKeys = ["Vx_{0}".format(i),"Vy_{0}".format(i), "Vz_{0}".format(i)]
                        
                        if key in positionKeys:
                            self.glWidget.simulation.objects[objectIndex].SetPosition(newValue,positionKeys.index(key))
                        elif key in velocityKeys:     
                            self.glWidget.simulation.objects[objectIndex].SetVelocity(newValue,velocityKeys.index(key))
                        elif key == "Mass_{0}".format(i):
                            self.glWidget.simulation.objects[objectIndex].SetMass(newValue)

                if not self.glWidget.isRealtime:
                    self.glWidget.precomputedData = []
                    self.glWidget.playbackIndex = 0
                    self.glWidget.refreshPointsAndPaths()
                    self.refreshPrecomputeControls()
        
        return setObjectValue     
     
    ###########################################################################################
    ### Gui Helper Functions                                                                ###
    ###########################################################################################     
   
    #connected to timer to be run automatically at a fixed interval              
    def timerRefresh(self):
        if not self.glWidget.isPaused:
            self.runtime.setText("Time: {0:.3f} years".format(round(self.glWidget.simulation.time,3)))
            self.refreshObjectValues()
    
    #run manually to fully update the glWidget and gui controls to latest state
    def refresh(self):
        self.glWidget.clearHistory()
        self.glWidget.update(0)
        self.runtime.setText("Time: {0:.3f} years".format(round(self.glWidget.simulation.time,3)))
        self.refreshObjectValues()
        self.refreshAccordion()  
        self.refreshPrecomputeControls()
            
    def refreshObjectValues(self):
        numObjects = len(self.glWidget.simulation.objects)        
        
        for i in range(5):
            objectIndex = 5*self.currentPage + i
            if self.ObjectAccordion.currentIndex() is i and objectIndex < numObjects: 
                
                currentControls = [self.ObjectControls["Mass_{0}".format(i)],
                                   self.ObjectControls["X_{0}".format(i)],
                                   self.ObjectControls["Y_{0}".format(i)],
                                   self.ObjectControls["Z_{0}".format(i)],
                                   self.ObjectControls["Vx_{0}".format(i)],
                                   self.ObjectControls["Vy_{0}".format(i)],
                                   self.ObjectControls["Vz_{0}".format(i)]]
                
                for control in currentControls:
                    control.blockSignals(True)
                
                self.ObjectControls["Mass_{0}".format(i)].setValue(self.glWidget.simulation.objects[objectIndex].mass)
                self.ObjectControls["X_{0}".format(i)].setValue(self.glWidget.simulation.objects[objectIndex].position[0])
                self.ObjectControls["Y_{0}".format(i)].setValue(self.glWidget.simulation.objects[objectIndex].position[1])
                self.ObjectControls["Z_{0}".format(i)].setValue(self.glWidget.simulation.objects[objectIndex].position[2])
                self.ObjectControls["Vx_{0}".format(i)].setValue(self.glWidget.simulation.objects[objectIndex].velocity[0])
                self.ObjectControls["Vy_{0}".format(i)].setValue(self.glWidget.simulation.objects[objectIndex].velocity[1])
                self.ObjectControls["Vz_{0}".format(i)].setValue(self.glWidget.simulation.objects[objectIndex].velocity[2])       
    
                for control in currentControls:
                    control.blockSignals(False)
    
    def refreshAccordion(self):
        numObjects = len(self.glWidget.simulation.objects)

        for i in range(5):
            objectIndex = 5*self.currentPage + i
            
            if objectIndex > numObjects - 1:
                self.ObjectAccordion.setItemEnabled(i,False)
                self.ObjectAccordion.setItemText(i,"")   
                self.ObjectAccordion.setItemIcon(i,QtGui.QIcon())
     
            else:
                self.ObjectAccordion.setItemEnabled(i,True)
                self.ObjectAccordion.setItemText(i,self.glWidget.simulation.objects[objectIndex].name)
                
                pixmap = QtGui.QPixmap(120,60)
                color = self.glWidget.simulation.objectSettings[objectIndex].color
                c = QtGui.QColor()
                c.setRgbF(color[0],color[1],color[2])
                pixmap.fill(c)
                self.ObjectAccordion.setItemIcon(i,QtGui.QIcon(pixmap))
    
    def refreshPrecomputeControls(self):
        if len(self.glWidget.precomputedData) == 0:
            self.computeStatus.setText("Status: no data")
            self.playbackPauseButton.setEnabled(False)
            self.playbackSpeed.setEnabled(False)
            self.playbackSlider.setEnabled(False)
        else:
            self.computeStatus.setText("Status: {0} timesteps computed.".format(len(self.glWidget.precomputedData)))
            self.playbackPauseButton.setEnabled(True)
            self.playbackSpeed.setEnabled(True)
            self.playbackSlider.setEnabled(True)
        
        self.playbackSlider.blockSignals(True)
        self.playbackSlider.setValue(self.glWidget.playbackIndex)
        self.playbackSlider.setMaximum(len(self.glWidget.precomputedData) - 1)
        self.playbackSlider.blockSignals(False)
        
    #update the dropdown list in view focus. Should be called when objects change
    def updateViewFocus(self):
        self.viewFocus.clear()
        self.viewFocus.addItem("None")
        for i in range(len(self.glWidget.simulation.objects)):
            self.viewFocus.addItem(self.glWidget.simulation.objects[i].name)
    
    #returnValues is set to True during init, so that the returned values can be used to init the glWidget
    def fromXml(self, xml, returnValues = False): 
        savedState = ElementTree.fromstring(xml)        
        
        newTime = savedState.findtext("Simulation/Time")
        if valid_float_string(newTime):
            newTime = float(newTime)
        
        newObjects = []
        newObjectSettings = []
        for objectElement in savedState.iterfind("Simulation/Objects/Object"):
            name = objectElement.findtext("Name")
            mass = objectElement.findtext("Mass")
            x = objectElement.findtext("Position/x")
            y = objectElement.findtext("Position/y")
            z = objectElement.findtext("Position/z")
            Vx = objectElement.findtext("Velocity/Vx")
            Vy = objectElement.findtext("Velocity/Vy")
            Vz = objectElement.findtext("Velocity/Vz")            
                
            #default to False if invalid or missing    
            showHistory = objectElement.findtext("Settings/ShowHistory") != "False"
            displayType = objectElement.findtext("Settings/DisplayType")
            
            if displayType not in ["Point","Image"]:
                displayType = "Point"

            color = []
            for c in objectElement.findtext("Settings/Color").split(','):
                #remove unwanted whitespace
                c = c.strip()
                if not valid_float_string(c):
                    continue
                elif float(c) >= 0 and float(c) <= 1:
                    c = float(c)
                else:
                    c = 1
                
                color.append(c)

            if len(color) != 3:
                color = [1,1,1]            
            
            if (name != "" and valid_float_string(mass) and
            valid_float_string(x) and valid_float_string(y) and valid_float_string(z) and
            valid_float_string(Vx) and valid_float_string(Vy) and valid_float_string(Vz)):
                newObjects.append(OrbitalObject(name, float(mass), numpy.array([float(x),float(y),float(z)]), numpy.array([float(Vx),float(Vy),float(Vz)])))
                newObjectSettings.append(ObjectSettings(displayType, showHistory, color))
            
        if not returnValues: 
            self.glWidget.simulation.time = newTime
            self.glWidget.simulation.objects = newObjects
            self.glWidget.simulation.objectSettings = newObjectSettings
            self.glWidget.simulation.state = self.glWidget.simulation.GetFlattenedObjects()
            
            self.refresh()
            self.updateViewFocus()
        else:
            return newObjects,newObjectSettings,newTime
        
    def toXml(self):   
        savedState = ElementTree.Element("SavedState")
        
        generalSettings = ElementTree.SubElement(savedState, "GeneralSettings")
        simulation = self.glWidget.simulation
        simulationElement = ElementTree.SubElement(savedState, "Simulation")
        
        time = ElementTree.SubElement(simulationElement, "Time")
        #TODO: find replacement for ElementTree. Problems:
        #1) I can't set the text in the constructor
        #2) Have to manually cast numbers as strings
        time.text = str(simulation.time)
        
        objects = ElementTree.SubElement(simulationElement, "Objects")
        
        for i,currentObject in enumerate(simulation.objects):
            objectElement = ElementTree.SubElement(objects, "Object")
            
            name = ElementTree.SubElement(objectElement, "Name")
            name.text = currentObject.name
            mass = ElementTree.SubElement(objectElement, "Mass")
            mass.text = str(currentObject.mass)
             
            position = ElementTree.SubElement(objectElement, "Position")
            x = ElementTree.SubElement(position, "x")
            x.text = str(currentObject.position[0])
            y = ElementTree.SubElement(position, "y")
            y.text = str(currentObject.position[1])
            z = ElementTree.SubElement(position, "z")
            z.text = str(currentObject.position[2])
             
            velocity = ElementTree.SubElement(objectElement, "Velocity")
            Vx = ElementTree.SubElement(velocity, "Vx")
            Vx.text = str(currentObject.velocity[0])
            Vy = ElementTree.SubElement(velocity, "Vy")
            Vy.text = str(currentObject.velocity[1])
            Vz = ElementTree.SubElement(velocity, "Vz")
            Vz.text = str(currentObject.velocity[2])
             
            settings = ElementTree.SubElement(objectElement, "Settings")
             
            showHistory = ElementTree.SubElement(settings, "ShowHistory")
            showHistory.text = str(simulation.objectSettings[i].showHistory)
            displayType = ElementTree.SubElement(settings, "DisplayType")
            displayType.text = str(simulation.objectSettings[i].displayType)
            color = ElementTree.SubElement(settings, "Color")
            #strip first/last chars because I don't want the  '[' and ']' that come from str(list)
            color.text = str(simulation.objectSettings[i].color)[1:-1]
            
        # 3) ElementTree can't pretty-print the xml, although minidom works fine as a replacement     
        return minidom.parseString(ElementTree.tostring(savedState, 'utf-8')).toprettyxml(indent="\t")   
    
    def objectsSetEnabled(self,enabled):
        controls = list(self.ObjectControls.values())
        for i in range(len(controls)):
            controls[i].setEnabled(enabled)    
            
#http://ssd.jpl.nasa.gov/horizons.cgi#top is very useful for getting accurate pos/vel data on many different objects
#It doesn't give the results in a format I need, so this is a convenience function to clean it up.
#assumes the input data is in km and km/day
def convertJplData():
    with open("resources/jpldata.txt", "r") as file:
        lines = file.readlines()
        jplData = ElementTree.Element("JplData")
        for i,line in enumerate(lines):
            positionLabels = ["x","y","z"]
            velocityLabels = ["Vx","Vy","Vz"]
            
            if (i+1)%2 != 0:
                position = ElementTree.SubElement(jplData, "Position")
                velocity = ElementTree.SubElement(jplData, "Velocity")                

            for j,data in enumerate(re.split(r'\s{1,}', line.strip())):
                if (i+1)%2 != 0: 
                    #convert from km to Gm
                    
                    data = float(data) / 10**6
                    pos = ElementTree.SubElement(position, positionLabels[j])
                    pos.text = str(data)
                else:
                    #convert from km/day to Gm/year
                    data = float(data) * 365 / 10**6
                    vel = ElementTree.SubElement(velocity, velocityLabels[j])
                    vel.text = str(data)

        print(minidom.parseString(ElementTree.tostring(jplData, 'utf-8')).toprettyxml(indent="\t"))
        
def main():
    app = QtGui.QApplication(sys.argv)
    
    with open("saves/Default.xml", "r") as file:
        xml = file.read() 

    widget = Main(xml)

    #widget can be implement in other layout
    sys.exit(app.exec_())

if __name__ == "__main__":
    #convertJplData()
    main()
