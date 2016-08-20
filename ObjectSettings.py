from PyQt4 import QtCore, QtGui, uic

ObjectSettingsWindow, ObjectSettingsUi = uic.loadUiType('objectSettings.ui')

class ObjectSettings:      
    def __init__(self, displayType, showHistory, color):
        self.showHistory = showHistory
        self.displayType = displayType
        self.color = color

class ObjectSettingsPopup(ObjectSettingsWindow, ObjectSettingsUi):
    def __init__(self,name,currentSettings):
        super(ObjectSettingsPopup, self).__init__()
        self.setupUi(self)
     
        self.displayTypeComboBox.addItem("Point")
     
        self.setWindowTitle(name)
        self.showHistoryCheckBox.setChecked(currentSettings.showHistory)
        
        self.show()
        
        #init after showing main popup so the sizes aren't 0
        self.initColorWidget(currentSettings.color)

        
    def initColorWidget(self,currentColor):
        #bit of a hack, since I couldn't find a way to get the QColorDialog as a widget easily embedded in the popup
        #instead I place the dialog in an mdi area and hide the fact that it is actually a window
        self.color = QtGui.QColorDialog()
        self.color.setCurrentColor(QtGui.QColor.fromRgbF(currentColor[0],currentColor[1],currentColor[2]))
        self.color.setOption(QtGui.QColorDialog.NoButtons)
        subWindow = self.mdiArea.addSubWindow(self.color)
        
        #hide the frame of the color dialog so it can't be moved around
        subWindow.setWindowFlags(QtCore.Qt.FramelessWindowHint)
        
        #size mdi area to match the color dialog
        self.mdiArea.setFixedSize(subWindow.size())
        
        #prevent resizing of the color dialog
        subWindow.setFixedSize(subWindow.size())
        self.color.show()               
        
    def getNewSettings(self):
        currentColor = self.color.currentColor()
        simpleColor = [currentColor.redF(),currentColor.greenF(),currentColor.blueF()]
        return ObjectSettings(self.displayTypeComboBox.currentText(),self.showHistoryCheckBox.isChecked(),simpleColor)
