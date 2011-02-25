#ifndef DSPOUTALSA_H_INCLUDED
#define DSPOUTALSA_H_INCLUDED

#include "voipmedia_global.h"

//#include <alsa/asoundlib.h>
#include "dspout.h"

//#include <Phonon>
//#include <Phonon/MediaObject>
//#include <Phonon/AudioOutput>
//#include <Phonon/ObjectDescription>
#include <QBuffer>
#include <QFile>



#include <QAudioFormat>
#include <QAudioOutput>
#include <QAudioInput>
#include <QAudioDeviceInfo>


class VOIPMEDIA_EXPORT DspOutAlsa : public DspOut
{
public:
  /**
  * Constructs a DspOutAlsa object representing the given
  * filename.  Default is default.
  */
  DspOutAlsa( const QString &fileName = "default" ); //Changed by bobosch

  /**
  * Destructor.  Will close the device if it is open.
  */
  virtual ~DspOutAlsa( void );

  bool openDevice( DeviceMode mode );
  bool writeBuffer( void );
  unsigned int readableBytes( void );
  bool readBuffer( int bytes );
  //int audio_fd;

private:
  //int err;

  //QString devname;         // device filename

  //QAudioFormat      settings;

  // ����� ����������� ������
  QAudioOutput* _pAudioOutput; // class member.
  QBuffer* _pOutputBuffer;

  // ���� ���������� ������
  QAudioInput* _pAudioInput; // class member.
  QBuffer* _pInputBuffer;

  QFile tempFileToRtp;

  QBuffer* tmpBuffer;

  //QFile outFile;
};

#endif  // DSPOUTOSS_H_INCLUDED
