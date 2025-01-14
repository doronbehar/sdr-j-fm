/*
 *    Copyright (C) 2008, 2009, 2010
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the SDR-J-FM
 *
 *    SDR-J-FM is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    SDR-J-FM is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with SDR-J-FM; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __FM_PROCESSOR_H
#define __FM_PROCESSOR_H

#include	<QThread>
#include	<QObject>
#include	<sndfile.h>
#include	<atomic>
#include	"fm-constants.h"
#include	"fir-filters.h"
#include	"fft-filters.h"
#include	"sincos.h"
#include	"pllC.h"
#include	"ringbuffer.h"
#include	"oscillator.h"

#include	"pilot-recover.h"
#include	"stereo-separation.h"
#include	"fm-demodulator.h"
#include	"rds-decoder.h"
#include	"newconverter.h"
#include	"squelchClass.h"
#include	"agc.h"

class deviceHandler;
class RadioInterface;
class audioSink;
class newConverter;

//#define USE_EXTRACT_LEVELS

#define	RDS_SAMPLE_DELAY	(2 * (FFT_SIZE - PILOTFILTER_SIZE))
template<typename T> class DelayLine {
public:
		DelayLine (const T & iDefault) : mDefault (iDefault) {
	   set_delay_steps (0); // reserve memory for at least one sample
	}

	void	set_delay_steps (const uint32_t iSteps) {
	   DataPtrIdx = 0;
	   DelayBuffer. resize (iSteps + 1, mDefault);
	}

	const T & get_set_value (const T & iVal) {
	   DelayBuffer [DataPtrIdx] = iVal;
	   DataPtrIdx = (DataPtrIdx + 1) % DelayBuffer. size();
	   return DelayBuffer [DataPtrIdx];
	}

private:
	uint32_t DataPtrIdx = 0;
	std::vector<T> DelayBuffer;
	T mDefault;
};



class fmProcessor : public QThread {
Q_OBJECT

public:
	enum class FM_Mode { Stereo, StereoPano, Mono };
	enum class ELfPlot { OFF, IF_FILTERED, DEMODULATOR, AF_SUM,
	                     AF_DIFF, AF_MONO_FILTERED, AF_LEFT_FILTERED,
	                     AF_RIGHT_FILTERED, RDS_INPUT, RDS_DEMOD };
	enum class ESqMode { OFF, NSQ, LSQ };
	enum Channels	{ S_STEREO, S_STEREO_SWAPPED, S_LEFT,
		             S_RIGHT, S_LEFTplusRIGHT,
		             S_LEFTminusRIGHT, S_LEFTminusRIGHT_Test };
	struct SMetaData {
	   enum class EPssState {OFF, ANALYZING, ESTABLISHED };
	   float DcValRf;
	   float DcValIf;  // used for AFC
	   float PssPhaseShiftDegree;
	   float PssPhaseChange;
	   EPssState PssState;
	   float GuiPilotStrength; // only valid if GUI scope shows the "Demodulation" signal
	   float PilotPllLockStrength;
	   bool  PilotPllLocked;
	};

public:
		fmProcessor (deviceHandler *,
	              RadioInterface *,
	              audioSink *,
	              fm_Demodulator *,
	              int32_t,                  // inputRate
	              int32_t,                  // decimation
	              int32_t,                  // workingRate
	              int32_t,                  // audioRate,
	              int,			// displaySize
	              int,			// spectrumSize
	              int32_t,                  // repeatRate
	              int,			// locale, Europe or US
	              RingBuffer<std::complex<float>> *,     // HFScope
	              RingBuffer<std::complex<float>> *,     // LFScope
	              RingBuffer<DSPCOMPLEX> *, // IQScope
	              int16_t);                 // threshold scanning
		~fmProcessor ();

	void		stop			();   // stop the processor
	void		setfmMode		(FM_Mode);
	void		setFMdecoder		(const QString &);
	void		setSoundMode		(uint8_t);
	void		setStereoPanorama	(int16_t iStereoPan);
	void		setSoundBalance		(int16_t);
	void		setDeemphasis		(int16_t);
	void		setVolume		(const float iVolGainDb);
	void		setlfcutoff		(int32_t);
	void		startDumping		(SNDFILE *);
	void		stopDumping		();
	void		setBandwidth		(const QString &);
	void		setAttenuation		(DSPFLOAT, DSPFLOAT);
	void		setfmRdsSelector	(rdsDecoder::ERdsMode);
	void		triggerFrequencyChange	();
	void		restartPssAnalyzer	();
	void		resetRds		();
	void		set_localOscillator	(int32_t);
	void		set_squelchMode		(ESqMode iSqMode);
	bool		getSquelchState		();
	void		setlfPlotType		(ELfPlot);
	void		setlfPlotZoomFactor	(int32_t);

	bool		isPilotLocked		(float &oLockStrength) const;
	void		setAutoMonoMode		(const bool iAutoMonoMode);
	void		setPSSMode		(const bool iPSSMode);
	void		setDCRemove		(const bool iDCREnabled);
	void		new_lfSpectrum 		();
	void		setTestTone		(const bool iTTEnabled);
	void		setDispDelay		(const int iDelay);

	float		get_demodDcComponent	();
	void		startScanning		();
	void		stopScanning		();
	void		set_squelchValue	(int16_t);
	void		set_ptyLocale		(int);
//
//	some private functions:
private:
	void		run			();
	void		processLfSpectrum	(std::vector<std::complex<float>> &, int, bool, bool);
	void		sendSampletoOutput	(DSPCOMPLEX);
	void		insertTestTone		(DSPCOMPLEX & ioS);
	void		evaluatePeakLevel	(const DSPCOMPLEX s);

	DSPFLOAT 	getSignal		(DSPCOMPLEX *, int32_t);
	DSPFLOAT 	getNoise		(DSPCOMPLEX *, int32_t);

//	the privates
private:
	rdsDecoder	myRdsDecoder;
	Oscillator	localOscillator;
	SinCos		mySinCos;
	AGC		pssAGC;
	DecimatingFIR	fmBand_1;
	DecimatingFIR	fmBand_2;
	fftFilter	fmAudioFilter;
	fftFilter	inputFilter;
	pilotRecovery	pilotRecover;
	PerfectStereoSeparation pPSS;
	fftFilter	rdsBandPassFilter;
	fftFilterHilbert rdsHilbertFilter;
	squelch		mySquelch;
	newConverter	theConverter;
	fm_Demodulator	*theDemodulator;
	float		rdsPhaseBuffer [RDS_SAMPLE_DELAY];
	int		rdsPhaseIndex;
	std::atomic<bool>	newAudioFilter;
	std::atomic<bool>	inputFilterOn;
	std::atomic<bool>	newInputFilter;

	int		displaySize;
	int		spectrumSize;
	deviceHandler	*myRig;
	RadioInterface	*myRadioInterface;
	audioSink	*theSink;
	int32_t		inputRate;    // typ. 2304 kSpS
	int32_t		fmRate;       // typ.  192 kSpS = InputRate / 12
	int32_t		workingRate;  // typ.   48 kSpS
	int32_t		audioRate;    // typ.   48 kSpS
	int		lowPassFrequency;
	std::atomic<bool>	fmAudioFilterActive;
	int32_t		fmBandwidth;
	int32_t		fmFilterDegree;

	int32_t		repeatRate;
	int		ptyLocale;
	bool		lfBuffer_newFlag;
	RingBuffer<std::complex<float>> *hfBuffer;
	RingBuffer<std::complex<float>> *lfBuffer;
	RingBuffer<std::complex<float>> *iqBuffer;
	std::vector<std::complex<float>> spectrumBuffer_lf;
	bool		scanning;
	int16_t		thresHold;

	ESqMode		squelchMode;
	int32_t		loFrequency;
	std::atomic<bool> running;
	bool		autoMono;
	bool		pssActive;

	int16_t		oldSquelchValue;
	int16_t		squelchValue;

	bool		dumping;
	SNDFILE		*dumpFile;

	int32_t		myCount;
	DSPFLOAT	Lgain;
	DSPFLOAT	Rgain;

	int32_t		peakLevelCurSampleCnt;
	int32_t		peakLevelSampleMax;
	DSPFLOAT	absPeakLeft;
	DSPFLOAT	absPeakRight;

	int32_t		suppressAudioSampleCntMax;
	int32_t		suppressAudioSampleCnt;

	struct TestTone {
	   bool		Enabled		= false;
	   float	TimePeriod	= 2.0f;
	   float	SignalDuration	= 0.025f;
	   uint32_t	TimePeriodCounter = 0;
	   uint32_t	NoSamplRemain	= 0;
	   float	CurPhase	= 0.0f;
	   float	PhaseIncr	= 0.0f;
	} testTone {};

	DelayLine<DSPCOMPLEX> delayLine {std::complex<float> (-40.0f, -40.0f)};

	void		process_signal_with_rds (const float,
	                                 std::complex<float> *,
	                                 std::complex<float> *);
//	RDS
	DSPFLOAT	pilotDelayPSS;
#ifdef DO_STEREO_SEPARATION_TEST
	DSPFLOAT	pilotDelay2;
#endif
	DSPCOMPLEX	audioGainCorrection (DSPCOMPLEX);
//
//	Volume
	DSPCOMPLEX	lastAudioSample;
	DSPFLOAT	deemphAlpha;
	DSPFLOAT	volumeFactor;

	DSPFLOAT	panorama;
	DSPFLOAT	leftChannel;    // -(balance - 50.0) / 100.0;;
	DSPFLOAT	rightChannel;   // (balance + 50.0) / 100.0;;
	FM_Mode		fmModus;
	uint8_t		soundSelector;

	rdsDecoder::ERdsMode rdsModus;

	bool		DCREnabled;
	DSPCOMPLEX	RfDC;

	ELfPlot		lfPlotType;
	bool		showFullSpectrum;
	int32_t		spectrumSampleRate;
	int32_t		zoomFactor;

	SMetaData	metaData	{};

signals:
	void		setPLLisLocked		(bool);
	void		hfBufferLoaded		();
	void		lfBufferLoaded		(bool, bool, int);
	void		iqBufferLoaded		();
	void		showMetaData(const SMetaData *);
	void		scanresult		();
	void		showPeakLevel		(const float, const float);
};

#endif

