from PyQt4 import QtCore, QtGui

import numpy
import re

_float_re = re.compile(r'(([+-]?\d+(\.\d*)?|\.\d+)([eE][+-]?\d+)?)')

def valid_float_string(string):
    match = _float_re.search(string)
    return match.groups()[0] == string if match else False

def format_float(value):
    """Modified form of the 'g' format specifier."""
    string = "{:g}".format(value).replace("e+", "e")
    string = re.sub("e(-?)0*(\d+)", r"e\1\2", string)
    return string   

class FloatValidator(QtGui.QValidator):

    def validate(self, string, position):
        if valid_float_string(string):
            return (self.Acceptable, string, position)
        if string == "" or string[position-1] in 'e.-+':
            return (self.Intermediate, string, position)
        return (self.Invalid, string, position)

    def fixup(self, text):
        match = _float_re.search(text)
        return match.groups()[0] if match else ""
        
class ScientificDoubleSpinBox(QtGui.QDoubleSpinBox):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.setMinimum(-numpy.inf)
        self.setMaximum(numpy.inf)
        self.validator = FloatValidator()
        self.setDecimals(1000)

    def mouseReleaseEvent(self, event):     
       if event.button() == QtCore.Qt.LeftButton:
           self.update()
           self.valueChanged.emit(self.value())
           event.accept()

    def validate(self, text, position):
        return self.validator.validate(text, position)

    def fixup(self, text):
        return self.validator.fixup(text)

    def valueFromText(self, text):
        return float(text)

    def textFromValue(self, value):
        return format_float(value)

    def stepBy(self, steps):
        text = self.cleanText()
        groups = _float_re.search(text).groups()
        decimal = float(groups[1])
        decimal += steps
        new_string = "{:g}".format(decimal) + (groups[3] if groups[3] else "")
        self.lineEdit().setText(new_string)
