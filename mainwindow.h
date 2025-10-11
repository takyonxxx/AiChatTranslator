#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtWidgets>
#include <QDir>
#include <QMediaRecorder>
#include <QStandardPaths>
#include <QSslSocket>
#include <QAudioDevice>
#include <QAudioSource>
#include <QAudioOutput>
#include <QAudioSink>
#include <QMediaFormat>
#include <QMediaCaptureSession>
#include <QStandardPaths>
#include <QNetworkAccessManager>
#include <QMediaDevices>
#include <QMediaPlayer>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextToSpeech>
#include <QTextDocument>
#include <QUrl>
#include "claudeai.h"

#ifdef __APPLE__
#include <AudioToolbox/AudioToolbox.h>
#endif

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
class QAudioBuffer;
QT_END_NAMESPACE

class AudioLevel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow();

    QByteArray generateSineWave(int durationInSeconds, int sampleRate, int channelCount, double frequency) {
        const int numFrames = durationInSeconds * sampleRate;
        const int numChannels = channelCount;

        QByteArray buffer;
        buffer.resize(numFrames * numChannels * sizeof(short));

        short* data = reinterpret_cast<short*>(buffer.data());
        const double amplitude = 0.5 * SHRT_MAX;

        for (int i = 0; i < numFrames; ++i) {
            double t = static_cast<double>(i) / sampleRate;
            for (int channel = 0; channel < numChannels; ++channel) {
                double value = amplitude * std::sin(2.0 * M_PI * frequency * t);
                data[i * numChannels + channel] = static_cast<short>(value);
            }
        }

        return buffer;
    }

public slots:
    void processBuffer(const QAudioBuffer&);
    QList<qreal> getBufferLevels(const QAudioBuffer&);

private slots:
    void toggleRecord();
    void toggleLanguage();
    void speechVoice();
    void translateText(QString, QString);
    void speakWithGoogleTTS(const QString &text, const QString &languageCode);
    void playGoogleTTSAudio(const QString &filePath);

    void localeChanged(const QLocale &locale);
    void voiceSelected(int index);

    void onStateChanged(QMediaRecorder::RecorderState);
    void onSpeechStateChanged(QTextToSpeech::State state);
    void onTtsPlayerStateChanged(QMediaPlayer::PlaybackState state);
    void updateProgress(qint64 pos);
    void micBufferReady();
    void displayErrorMessage();
    void inputDeviceChanged(int index);
    void outputDeviceChanged(int index);
    void appendText(QString text);
    void setOutputFile();

    void sslErrors(const QList<QSslError> &errors);
    void httpSpeechFinished();
    void httpTranslateFinished();
    void httpAiFinished();
    void httpGoogleTtsFinished();
    void httpSpeechReadyRead();
    void httpTranslateReadyRead();
    void httpGoogleTtsReadyRead();
    void handleAudioStateChanged(QAudio::State);
    void requestMicrophonePermission();
    void initializeAi();

    void on_exitButton_clicked();
    void on_clearButton_clicked();
    void on_recordButton_clicked();

    void on_micVolumeSlider_valueChanged(int value);
    void on_speechVolumeSlider_valueChanged(int value);
    void on_voxSensivitySlider_valueChanged(int value);
    void on_languageButton_clicked();

    void onSilenceDetected();

    void onClaudeResponse(const QString &response);
    void onClaudeError(const QString &error);

    void on_enableClaude_toggled(bool checked);

private:
#ifdef __APPLE__
    void startRecording();
    void stopRecording();
    void recordBuffer(const QByteArray&);
    bool isBufferFull() const;
    void writeAudioToFile(const QByteArray&);
    AudioFileID audioFile;
    CFURLRef fileURL;
    QByteArray accumulatedBuffer;
#endif
    void clearAudioLevels();
    void setSpeechEngine();
    QMediaFormat selectedMediaFormat() const;

    ClaudeAI *m_claudeAI = nullptr;
    int m_aIMaxWords = 20;
    bool aiMode = false;

    QTextToSpeech *m_speech = nullptr;
    QVector<QVoice> m_voices;
    int m_current_language_index{0};
    int m_current_voice_index{0};
    QString langpair;
    QString languageCode;

    QMediaCaptureSession m_captureSession;
    QMediaRecorder *m_audioRecorder = nullptr;
    QAudioSource *m_audioInputSource = nullptr;
    QAudioSink *m_audioOutput;
    QAudioFormat m_format;
    QAudioInput* m_audioInput = nullptr;
    QIODevice *ioInputDevice = nullptr;
    QIODevice *ioOutputDevice = nullptr;
    QList<AudioLevel*> m_audioLevels;
    QAudioFormat audio_format;

    QMediaPlayer *m_ttsPlayer = nullptr;
    QAudioOutput *m_ttsAudioOutput = nullptr;

    bool m_recording = false;
    bool m_speaking = false;
    bool m_record_started = false;
    bool m_outputLocationSet = false;
    bool m_qlearning = false;
    bool m_translate = false;
    float m_vox_sensitivity = 0.25;

    QDateTime m_lastVoiceDetectedTime;  // Son ses algılama zamanı
    QTimer *m_silenceTimer = nullptr;   // Sessizlik timer'ı
    bool m_voiceDetectedInCurrentRecording = false;

    const int maxDuration = 30000;  // ✅ 5000'den 10000'e (max 10 saniye)
    const int minDuration = 1500;
    const int minRecordDuration = 1500;  // ✅ 1500'den 500'e düşür
    const int SILENCE_TIMEOUT_MS = 2500;  // ✅ 1 saniye sessizlik
    const int SPEECH_COOLDOWN_MS = 1500;
    const int VOX_DEBOUNCE_MS = 500;
    QDateTime m_lastSpeechEndTime;
    QDateTime m_lastVoxTriggerTime;
    const unsigned sampleRate = 16000;
    const unsigned channelCount = 1;
    int recordDuration = 0;

    QNetworkAccessManager *qnam;
    QUrl urlSpeech;
    QUrl urlSearch;
    QUrl urlLanguageTranslate;
    QUrl urlAi;
    QUrl urlGoogleTts;
    QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> speech_reply;
    QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> translate_reply;
    QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> ai_reply;
    QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> google_tts_reply;

    const QString fileName = "record";
    QString ext = "";
    QFile file;   

    Ui::MainWindow *ui = nullptr;
};

#endif // MAINWINDOW_H
