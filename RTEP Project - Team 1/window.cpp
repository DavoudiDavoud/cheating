#include "window.h"
#include "adcreader.h"

#include <algorithm> // Std stuff.
#include <cmath> // Is this still used?
#include <complex.h> // Add complex number support.
#include <fftw3.h> // DFT stuff.

Window::Window() : count(0)
{
	// Set up the filter select knob.
	knob_fs = new QwtKnob;
	setting_fs = 0;
	knob_fs->setScale(0, 3);
	knob_fs->setValue(0);
	connect( knob_fs, SIGNAL(valueChanged(double)), SLOT(setFilterSelection(double)) );

	// Set up the gain knob.
	knob_gain = new QwtKnob;
	setting_gain = 0;
	knob_gain->setScale(0, 7);
	knob_gain->setValue(0);
	connect( knob_gain, SIGNAL(valueChanged(double)), SLOT(setGain(double)) );

	// Set up the length knob.
	knob_length = new QwtKnob;
	setting_length = 5.0;
	knob_length->setScale(1, 10);
	knob_length->setValue(5);
	connect( knob_length, SIGNAL(valueChanged(double)), SLOT(setLength(double)) );

	// DFT button.
	button_dft = new QCheckBox("Frequency Spectrum");
	dft_f = new double[1];
	dft_c1 = new double[1];
	dft_c2 = new double[1];
	dft_on = false;
	connect( button_dft, SIGNAL(stateChanged(int)), SLOT(dftMode(int)) );

	// 1C button.
	button_1c = new QCheckBox("Single Channel");
	setting_1c = false;
	connect( button_1c, SIGNAL(stateChanged(int)), SLOT(singleCMode(int)) );


	// Plot data initalizatqframeion.
	plotResize = false;
	plotResample = false;
	plotFreq = 8.333333333;
	plotDataSize = 41;
	
	xData = new double[plotDataSize];
	yData = new double[plotDataSize];
	yData2 = new double[plotDataSize];
	for( uint32_t index = 0; index < plotDataSize; index+= 1 ){
		xData[index] = (plotDataSize - index - 1) * ( setting_length / (plotDataSize - 1.0) );
		yData[index] = 0;
		yData2[index] = 0;
	}

	// Channel 1 plot.
	curve = new QwtPlotCurve("Curve 1");
	plot = new QwtPlot;
	curve->setSamples(xData, yData, plotDataSize);
	curve->attach(plot);
	plot->replot();
	plot->show();

	// Channel 2 plot.
	curve2 = new QwtPlotCurve("Curve 2");
	plot2 = new QwtPlot;
	curve2->setSamples(xData, yData2, plotDataSize);
	curve2->attach(plot2);
	plot2->replot();
	plot2->show();

	plotBusy = false;


	// Set up the layout - top is filter section,  then gain, then length, then dft.
	vLayout = new QVBoxLayout;
	vLayout->addWidget(knob_fs);
	vLayout->addWidget(knob_gain);
	vLayout->addWidget(knob_length);
	vLayout->addWidget(button_dft);
	vLayout->addWidget(button_1c);

	// Plot 1 above Plot 2
	vLayout2 = new QVBoxLayout;
	vLayout2->addWidget(plot);
	vLayout2->addWidget(plot2);

	// Plots go to the right if knobs.
	hLayout = new QHBoxLayout;
	hLayout->addLayout(vLayout);
	hLayout->addLayout(vLayout2);

	setLayout(hLayout);
	// This is a demo for a thread which can be
	// used to read from the ADC asynchronously.
	// At the moment it doesn't do anything else than
	// running in an endless loop and which prints out "tick"
	// every second.
	adcreader = new ADCreader();
	adcreader->start();
}

Window::~Window() {
	// tells the thread to no longer run its endless loop
	adcreader->quit();
	// wait until the run method has terminated
	adcreader->wait();
	delete adcreader;
}

void Window::timerEvent( QTimerEvent * )
{
	// Check we are not modifying the plot more than once at a time (allows for frame skips if processor is busy).
	if( plotBusy ) return;
	plotBusy = true;

	if( plotResize ){
		resizePlots();
		plotResize = false;
	}

	if( plotResample ){
		resamplePlots();
		plotResample = false;
	}

	//uint32_t countold = cqframeount;
	uint32_t number = adcreader->appendResults(yData, plotDataSize, 0);
	count+= number;

	number = adcreader->appendResults(yData2, plotDataSize, 1);
	count+= number;

	//fprintf(stderr, "number of samples: %d\n", count - countold);

	if( dft_on ){
		// Some variables.
		fftw_plan dft;
		uint32_t freqs = plotDataSize / 2 + 1;
		fftw_complex* datafc1 = fftw_alloc_complex(freqs);
		fftw_complex* datafc2 = fftw_alloc_complex(freqs);

		// Transform current data into frequency space.
		dft = fftw_plan_dft_r2c_1d((int)plotDataSize, yData, datafc1, FFTW_ESTIMATE | FFTW_PRESERVE_INPUT); // Raspberry can only estimate.
		fftw_execute(dft);
		fftw_destroy_plan(dft);

		if( !setting_1c ){
			dft = fftw_plan_dft_r2c_1d((int)plotDataSize, yData2, datafc2, FFTW_ESTIMATE | FFTW_PRESERVE_INPUT); // Raspberry can only estimate.
			fftw_execute(dft);
			fftw_destroy_plan(dft);
		}

		delete dft_f;
		delete dft_c1;
		delete dft_c2;

		dft_f = new double[freqs];
		dft_c1 = new double[freqs];
		dft_c2 = new double[freqs];

		double scale = (plotFreq / 2) / (double)(freqs - 1);
		for( uint32_t i = 0 ; i < freqs ; i+= 1 ){
			dft_f[i] = (double)i * scale;
			dft_c1[i] = cabs(datafc1[i]);
			dft_c2[i] = cabs(datafc2[i]);
		}

		fftw_free(datafc1);
		fftw_free(datafc2);

		curve->setSamples(dft_f, dft_c1, freqs);
		if( !setting_1c ) curve2->setSamples(dft_f, dft_c2, freqs);
	}else{
		curve->setSamples(xData, yData, plotDataSize);
		if( !setting_1c ) curve2->setSamples(xData, yData2, plotDataSize);
	}

	plot->replot();
	if( !setting_1c ) plot2->replot();

	plotBusy = false;
}

void Window::resizePlots(  ){
	// Get new length;
	uint32_t newsize = (uint32_t)(setting_length * plotFreq);

	// Resize if not already the required size.
	if( newsize != plotDataSize ){
		double* temp1 = new double[newsize];
		double* temp2 = new double[newsize];
		double* temp3 = new double[newsize];
		double scale = 1.0 / plotFreq; // Cached scale.
		int32_t offset = newsize - plotDataSize;
		if( newsize > plotDataSize ){
			for( uint32_t i = 0 ; i < offset ; i+= 1 ){
				temp1[i] = 0.0;
				temp2[i] = 0.0;
				temp3[i] = ((double)(newsize - i) - 1.0) * scale;
			}
		}
		if( newsize < plotDataSize ){
			for( uint32_t i = 0 ; i < newsize ; i+= 1 ){
				temp1[i] = yData[i + plotDataSize - newsize];
				temp2[i] = yData2[i + plotDataSize - newsize];
				temp3[i] = xData[i + plotDataSize - newsize];
			}
		}else{
			for( uint32_t i = offset ; i < newsize ; i+= 1 ){
				temp1[i] = yData[i - offset];
				temp2[i] = yData2[i - offset];
				temp3[i] = xData[i - offset];
			}
		}

		delete[] yData;
		delete[] yData2;
		delete[] xData;
		yData = temp1;
		yData2 = temp2;
		xData = temp3;

		plotDataSize = newsize;
	}
}

void Window::resamplePlots(  ){
	// Get channel frequency.
	uint32_t f1, f2;
	f1 = adcreader->getSampleRate(0);
	f2 = adcreader->getSampleRate(1);

	// Calculate actual frequency (due to multi channel sampling) and samples needed.
	double f;
	if( setting_1c ){
		f = (double)f1;
	}else{
		f = 1.0 / ((1.0 / (double)f1 + 1.0 / (double)f2) * 3.0);
	}

	plotFreq = f;
	uint32_t newsize = (uint32_t)(setting_length * f);

	// Resize if not already the required size.
	if( newsize != plotDataSize ){
		// Some variables.
		fftw_plan dft;
		uint32_t freqs = (newsize > plotDataSize ? newsize : plotDataSize) / 2 + 1;
		fftw_complex* dataf = fftw_alloc_complex(freqs);

		// Transform current data into frequency space.
		dft = fftw_plan_dft_r2c_1d((int)plotDataSize, yData, dataf, FFTW_ESTIMATE); // Raspberry can only estimate.
		fftw_execute(dft);
		fftw_destroy_plan(dft);
		delete[] yData;

		// Transform data into new sample space.
		for( uint32_t i = plotDataSize / 2 + 1 ; i < freqs ; i+= 1 ){
			dataf[i] = 0.0 + 0.0 * I;
		}
		yData = new double[newsize];
		dft = fftw_plan_dft_c2r_1d((int)newsize, dataf, yData, FFTW_ESTIMATE);
		fftw_execute(dft);
		fftw_destroy_plan(dft);

		// Normalize new samples.
		for( uint32_t i = 0 ; i < newsize ; i+= 1 ){
			yData[i]/= (double)newsize;
		}

		if( !setting_1c ){
		//Repeat for channel 2.
			dft = fftw_plan_dft_r2c_1d((int)plotDataSize, yData2, dataf, FFTW_ESTIMATE); // Raspberry can only estimate.
			fftw_execute(dft);
			fftw_destroy_plan(dft);
		}
		delete[] yData2;

		for( uint32_t i = plotDataSize / 2 + 1 ; i < freqs ; i+= 1 ){
			dataf[i] = 0.0 + 0.0 * I;
		}
		yData2 = new double[newsize];
		dft = fftw_plan_dft_c2r_1d((int)newsize, dataf, yData2, FFTW_ESTIMATE);
		fftw_execute(dft);
		fftw_destroy_plan(dft);

		for( uint32_t i = 0 ; i < newsize ; i+= 1 ){
			yData2[i]/= (double)newsize;
		}

		// Finish off resize.
		fftw_free(dataf);
		plotDataSize = newsize;

		// Generate new scale.
		delete[] xData;
		xData = new double[plotDataSize];
		double scale = 1.0 / plotFreq; // Cached scale.
		for( uint32_t index = 0; index < plotDataSize; index+= 1 ){
			xData[index] = (plotDataSize - index - 1) * scale;
		}
	}
}

void Window::setFilterSelection(double filter)
{
	// Convert selection and update.
	uint8_t fs = (uint8_t)(filter*3.0/10.0);
	if( fs != setting_fs ){
		// Update filter selection.
		setting_fs = fs;
		adcreader->setFilter(fs, 0);
		adcreader->setFilter(fs, 1);
		plotResample = true;
		fprintf(stderr, "filter selection: %d\n", fs);
	}
}

void Window::setGain(double gain)
{
	// Convert selection and update.
	uint8_t g = (uint8_t)(gain*7.0/10.0);
	if( g != setting_gain ){
		// Update filter selection.
		setting_gain = g;
		adcreader->setGain(g, 0);
		adcreader->setGain(g, 1);
		fprintf(stderr, "gain selection: %d\n", g);
	}
}


void Window::setLength(double length){
	// Convert selection and update.
	uint8_t select = (uint8_t)(length * 9.0 / 10.0 + 1.0);
	double newlength = (double)select;
	if( newlength != setting_length ){
		// Update length.
		setting_length = newlength;
		plotResize = true;
		fprintf(stderr, "length selection: %f seconds\n", newlength);
	}
}

void Window::dftMode(int state){
	fprintf(stderr, "dft mode: toggle\n");
	dft_on = !dft_on;
}

void Window::singleCMode(int state){
	fprintf(stderr, "single channel mode: toggle\n");
	setting_1c = !setting_1c;
	if( setting_1c ){
		adcreader->samplingEnable(false, 1);
	}else{
		adcreader->samplingEnable(true, 1);
	}
	plotResample = true;
}
