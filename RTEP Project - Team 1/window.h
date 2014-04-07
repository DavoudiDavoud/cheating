#ifndef WINDOW_H
#define WINDOW_H

#include <qwt/qwt_thermo.h>
#include <qwt/qwt_knob.h>
#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_curve.h>

#include <QBoxLayout>
#include <QCheckBox>

#include "adcreader.h"

// class definition 'Window'
class Window : public QWidget
{
	// must include the Q_OBJECT macro for for the Qt signals/slots framework to work with this class
	Q_OBJECT

public:
	Window(); // default constructor - called when a Window is declared without arguments

	~Window();

	void timerEvent( QTimerEvent * );
	void resizePlots(  );
	void resamplePlots(  );

public slots:
	void setFilterSelection(double filter);
	void setGain(double gain);
	void setLength(double length);
	void dftMode(int state);
	void singleCMode(int state);

// internal variables for the window class
private:
	QwtKnob      *knob_fs;
	QwtKnob      *knob_gain;
	QwtKnob      *knob_length;
	QCheckBox    *button_dft;
	QCheckBox    *button_1c;
	QwtPlot      *plot;
	QwtPlot      *plot2;
	QwtPlotCurve *curve;
	QwtPlotCurve *curve2;

	// layout elements from Qt itself http://qt-project.org/doc/qt-4.8/classes.html
	QVBoxLayout  *vLayout;  // vertical layout
	QVBoxLayout  *vLayout2;  // vertical layout2
	QHBoxLayout  *hLayout;  // horizontal layout

	uint8_t setting_fs;
	uint8_t setting_gain;
	double setting_length; // The length of the plot in seconds.
	bool setting_1c;
	bool plotResize;       // Orders the update thread to resize plot.
	bool plotResample;       // Orders the update thread to resize plot.
	double plotFreq;
	volatile bool plotBusy; // The plot is in the process of bing manipulated. 
	uint32_t plotDataSize; // The number of samples.

	// data arrays for the plot
	double* xData;
	double* yData;
	double* yData2;

	// Data arrays for DFT view.
	double* dft_f;
	double* dft_c1;
	double* dft_c2;
	bool dft_on;

	int count;

	ADCreader *adcreader;
};

#endif // WINDOW_H
