#include "mainwindow.h"
#include "audiolevel.h"
#include "ui_mainwindow.h"
#include <qmediadevices.h>
#include <qmediaformat.h>
#include <qaudiodevice.h>
#include <qaudiobuffer.h>
#include <qaudioinput.h>
#include <qimagecapture.h>
#include <qmediaformat.h>
#include <QMimeType>
#include "constants.h"
#include <QCoreApplication>
#include <QFuture>
#include <QFutureWatcher>

#ifdef __aarch64__
#include <QJniObject>
#include <QtCore/private/qandroidextras_p.h>
#endif

static QVariant boxValue(const QComboBox *box)
{
    int idx = box->currentIndex();
    if (idx == -1)
        return QVariant();
    return box->itemData(idx);
}

MainWindow::MainWindow()
    : ui(new Ui::MainWindow)
{
#ifdef __aarch64__
    requestMicrophonePermission();
#else
    initializeAi();
#endif
}

MainWindow::~MainWindow()
{
    delete m_speech;
    delete m_audioOutput;
    delete ioInputDevice;
    delete ioOutputDevice;
    delete m_audioRecorder;
    delete m_audioInputSource;
    delete m_ttsPlayer;
    delete m_ttsAudioOutput;
    delete qnam;
}

void MainWindow::initializeAi()
{
    ui->setupUi(this);

#ifdef Q_OS_IOS
    QSize screenSize = qApp->primaryScreen()->availableSize();
    int targetWidth = screenSize.width() * 0.8;
    int targetHeight = screenSize.height() * 0.8;
    this->setFixedSize(targetWidth, targetHeight);
#endif

    ui->recordButton->setStyleSheet("font-size: 16pt; font-weight: bold; color: white;background-color:#900C3F; padding: 6px; spacing: 6px;");
    ui->languageButton->setStyleSheet("font-size: 16pt; font-weight: bold; color: white;background-color:#900C3F; padding: 6px; spacing: 6px;");
    ui->clearButton->setStyleSheet("font-size: 16pt; font-weight: bold; color: white;background-color:#045F0A; padding: 6px; spacing: 6px;");
    ui->exitButton->setStyleSheet("font-size: 16pt; font-weight: bold; color: white;background-color:#045F0A; padding: 6px; spacing: 6px;");
    ui->textTerminal->setStyleSheet("font: 13pt; color: #00cccc; background-color: #001a1a;");
    ui->labelOutputDevice->setStyleSheet("font-size: 14pt; font-weight: bold; color: white;background-color:#154360; padding: 6px; spacing: 6px;");
    ui->audioOutputDeviceBox->setStyleSheet("font-size: 14pt; font-weight: bold; color: white;background-color:#6B0785; padding: 6px; spacing: 6px;");
    ui->labelInputDevice->setStyleSheet("font-size: 14pt; font-weight: bold; color: white;background-color:#154360; padding: 6px; spacing: 6px;");
    ui->audioInputDeviceBox->setStyleSheet("font-size: 14pt; font-weight: bold; color: white;background-color:#6B0785; padding: 6px; spacing: 6px;");
    ui->labelRecordTime->setStyleSheet("font-size: 14pt; font-weight: bold; color: white;background-color:#154360; padding: 6px; spacing: v;");
    ui->recordTimeBox->setStyleSheet("font-size: 14pt; font-weight: bold; color: white;background-color:#6B0785; padding: 6px; spacing: 6px;");
    ui->labelMicLevelInfo->setStyleSheet("font-size: 14pt; font-weight: bold; color: white;background-color:#154360; padding: 6px; spacing: 6px;");
    ui->labelSpeechLevelInfo->setStyleSheet("font-size: 14pt; font-weight: bold; color: white;background-color:#154360; padding: 6px; spacing: 6px;");
    appendText(tr("Device supports OpenSSL: %1").arg((QSslSocket::supportsSsl()) ? "Yes" : "No"));

#ifndef __APPLE__
    m_audioRecorder = new QMediaRecorder(this);
    connect(m_audioRecorder, &QMediaRecorder::durationChanged, this, &MainWindow::updateProgress);
    connect(m_audioRecorder, &QMediaRecorder::recorderStateChanged, this, &MainWindow::onStateChanged);
    connect(m_audioRecorder, &QMediaRecorder::errorOccurred, this, &MainWindow::displayErrorMessage);
    connect(m_audioRecorder, &QMediaRecorder::errorChanged, this, &MainWindow::displayErrorMessage);
#endif

    QAudioDevice outputDevice;
    for (auto &device: QMediaDevices::audioOutputs()) {
        outputDevice = device;
        break;
    }

    m_format.setSampleFormat(QAudioFormat::Int16);
    m_format.setSampleRate(sampleRate);
    m_format.setChannelCount(channelCount);

    if (outputDevice.isFormatSupported(m_format)) {
        m_audioOutput = new QAudioSink(outputDevice, m_format, this);
        connect(m_audioOutput,&QAudioSink::stateChanged, this, &MainWindow::handleAudioStateChanged);
    }

    m_audioInput = new QAudioInput(this);

    m_ttsPlayer = new QMediaPlayer(this);
    m_ttsAudioOutput = new QAudioOutput(this);
    m_ttsPlayer->setAudioOutput(m_ttsAudioOutput);
    m_ttsAudioOutput->setVolume(1.0);
    connect(m_ttsPlayer, &QMediaPlayer::playbackStateChanged, this, &MainWindow::onTtsPlayerStateChanged);

    m_speech = new QTextToSpeech(this);
    connect(m_speech, &QTextToSpeech::stateChanged, this, &MainWindow::onSpeechStateChanged);
    connect(m_speech, &QTextToSpeech::localeChanged, this, &MainWindow::localeChanged);

    langpair = "tr|en";
    languageCode = "tr-TR";

    for (auto device: QMediaDevices::audioInputs()) {
        auto name = device.description();
        ui->audioInputDeviceBox->addItem(name, QVariant::fromValue(device));
    }
    connect(ui->audioInputDeviceBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::inputDeviceChanged);

    for (auto &device: QMediaDevices::audioOutputs()) {
        auto name = device.description();
        ui->audioOutputDeviceBox->addItem(name, QVariant::fromValue(device));
    }
    connect(ui->audioOutputDeviceBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::outputDeviceChanged);

    ui->recordTimeBox->addItem(QStringLiteral("1000"), QVariant(1000));
    ui->recordTimeBox->addItem(QStringLiteral("3000"), QVariant(3000));
    ui->recordTimeBox->addItem(QStringLiteral("5000"), QVariant(5000));
    ui->recordTimeBox->setCurrentIndex(1);

    qnam = new QNetworkAccessManager(this);

    this->urlSpeech.setUrl(speechBaseApi);
    this->urlLanguageTranslate.setUrl(translateUrl);
    this->urlAi.setUrl(aiUrl);
    this->urlGoogleTts.setUrl(googleTtsBaseApi);

    connect(ui->language, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &MainWindow::languageSelected);

    setSpeechEngine();
    inputDeviceChanged(0);
    outputDeviceChanged(0);

    if (ui->micVolumeSlider) {
        ui->micVolumeSlider->setValue(80);
    }

    if (ui->voxSensivitySlider) {
        ui->voxSensivitySlider->setValue(35);
    }

    m_lastSpeechEndTime = QDateTime::currentDateTime().addSecs(-10);

    // ✅ Başlangıç buton text'ini ayarla
    ui->languageButton->setText("TR→EN");

    appendText("🎤 Sistem hazır. Konuşmaya başlayın...");
    appendText("🔄 Mod: Türkçe dinleme → İngilizce konuşma");
    speakWithGoogleTTS("Lütfen, kayıt düğmesine basınız", "tr-TR");
}

void MainWindow::requestMicrophonePermission()
{
#ifdef __aarch64__
    if (QNativeInterface::QAndroidApplication::sdkVersion() >= 23) {
        QFuture<QtAndroidPrivate::PermissionResult> checkFuture = QtAndroidPrivate::checkPermission(QString("android.permission.RECORD_AUDIO"));
        QFutureWatcher<QtAndroidPrivate::PermissionResult>* checkWatcher = new QFutureWatcher<QtAndroidPrivate::PermissionResult>(this);

        connect(checkWatcher, &QFutureWatcher<QtAndroidPrivate::PermissionResult>::finished, this, [this, checkWatcher]() {
            QtAndroidPrivate::PermissionResult checkResult = checkWatcher->result();
            if (checkResult == QtAndroidPrivate::PermissionResult::Denied) {
                QFuture<QtAndroidPrivate::PermissionResult> requestFuture = QtAndroidPrivate::requestPermission(QString("android.permission.RECORD_AUDIO"));
                QFutureWatcher<QtAndroidPrivate::PermissionResult>* requestWatcher = new QFutureWatcher<QtAndroidPrivate::PermissionResult>(this);

                connect(requestWatcher, &QFutureWatcher<QtAndroidPrivate::PermissionResult>::finished, this, [this, requestWatcher]() {
                    QtAndroidPrivate::PermissionResult requestResult = requestWatcher->result();
                    if (requestResult == QtAndroidPrivate::PermissionResult::Authorized) {
                        initializeAi();
                    } else {
                        qDebug() << "ERROR: Microphone permission denied";
                        QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);
                    }
                    requestWatcher->deleteLater();
                });

                requestWatcher->setFuture(requestFuture);
            } else if (checkResult == QtAndroidPrivate::PermissionResult::Authorized) {
                initializeAi();
            }
            checkWatcher->deleteLater();
        });

        checkWatcher->setFuture(checkFuture);
    } else {
        initializeAi();
    }
#endif
}

void MainWindow::handleAudioStateChanged(QAudio::State newState)
{
    switch (newState) {
    case QAudio::IdleState:
        m_audioOutput->stop();
        break;
    case QAudio::StoppedState:
        if (m_audioOutput->error() != QAudio::NoError) {
            qDebug() << "ERROR: Audio output error";
        }
        break;
    default:
        break;
    }
}

void MainWindow::setOutputFile()
{
    QString filePath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QDir folderDir(filePath);
    QStringList files = folderDir.entryList(QDir::Files);
    for (const QString &file : files) {
        QString file_Path = filePath + QDir::separator() + file;
        QFile::remove(file_Path);
    }

    m_audioRecorder->setOutputLocation(QUrl::fromLocalFile(filePath.remove("file://") + "/record"));
    m_outputLocationSet = true;
}

void MainWindow::setSpeechEngine()
{
    ui->language->blockSignals(true);
    ui->language->clear();

    const QVector<QLocale> locales = m_speech->availableLocales();
    int counter = 0;

    for (const QLocale &locale : locales) {
        QString name(QString("%1 (%2)")
                         .arg(QLocale::languageToString(locale.language()))
                         .arg(QLocale::territoryToString(locale.territory())));
        QVariant localeVariant(locale);
        ui->language->addItem(name, localeVariant);

        if (langpair == "tr|en" && name.contains("English (United States)"))
        {
            m_current_language_index = counter;
        }
        else if (langpair == "en|tr" && name.contains("Turkish"))
        {
            m_current_language_index = counter;
        }
        counter++;
    }

    ui->language->setCurrentIndex(m_current_language_index);
    ui->language->blockSignals(false);
}

void MainWindow::languageSelected(int language)
{
    QLocale locale = ui->language->itemData(language).toLocale();
    m_speech->setLocale(locale);
    localeChanged(locale);
}

void MainWindow::localeChanged(const QLocale &locale)
{
    QVariant localeVariant(locale);
    ui->language->setCurrentIndex(ui->language->findData(localeVariant));

    disconnect(ui->voice, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &MainWindow::voiceSelected);
    ui->voice->clear();
    int counter=0;

    m_voices = m_speech->availableVoices();
    for (const QVoice &voice : std::as_const(m_voices)) {
        ui->voice->addItem(QString("%1 - %2 - %3").arg(voice.name())
                               .arg(QVoice::genderName(voice.gender()))
                               .arg(QVoice::ageName(voice.age())));
        if (QVoice::genderName(voice.gender()).contains("Female") || voice.name().contains("female"))
        {
            m_current_voice_index = counter;
        }
        counter++;
    }
    connect(ui->voice, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &MainWindow::voiceSelected);
    ui->voice->setCurrentIndex(m_current_voice_index);
}

void MainWindow::voiceSelected(int index)
{
    m_speech->setVoice(m_voices.at(index));
}

void MainWindow::sslErrors(const QList<QSslError> &errors)
{
    QString errorString;
    for (const QSslError &error : errors) {
        if (!errorString.isEmpty())
            errorString += '\n';
        errorString += error.errorString();
    }

    if (QMessageBox::warning(this, tr("SSL Errors"),
                             tr("One or more SSL errors has occurred:\n%1").arg(errorString),
                             QMessageBox::Ignore | QMessageBox::Abort) == QMessageBox::Ignore) {
        ai_reply->ignoreSslErrors();
    }
}

void MainWindow::httpSpeechFinished()
{
    if(speech_reply->error() != QNetworkReply::NoError)
    {
        qDebug() << "ERROR: Speech API:" << speech_reply->errorString();
        appendText("❌ Speech API Error");
        speech_reply.reset();
        return;
    }

    // ✅ Tüm data'yı burada oku
    auto data = QJsonDocument::fromJson(speech_reply->readAll());
    m_recording = false;

    auto error = data["error"]["message"];

    if (error.isUndefined()) {
        auto command = data["results"][0]["alternatives"][0]["transcript"].toString();

        if (command.size() > 0) {
            appendText("📝 " + command);
            translateText(command, langpair);
        }
        else {
            appendText("⚠️ Ses algılanamadı");
        }
    } else {
        qDebug() << "ERROR: Speech recognition:" << error.toString();
        appendText("❌ Ses tanıma hatası");
    }

    speech_reply.reset();
}

void MainWindow::httpSpeechReadyRead()
{

}


void MainWindow::httpTranslateFinished()
{
    if(translate_reply->error() != QNetworkReply::NoError)
    {
        qDebug() << "ERROR: Translate API:" << translate_reply->errorString();
        appendText("❌ Translation Error");
        translate_reply.reset();
        return;
    }

    QString strReply = translate_reply->readAll().toStdString().c_str();
    QJsonDocument jsonResponseDoc = QJsonDocument::fromJson(strReply.toUtf8());

    if (!jsonResponseDoc.isNull() && jsonResponseDoc.isObject()) {
        QJsonObject responseObject = jsonResponseDoc.object();
        if (responseObject.contains("responseData") && responseObject["responseData"].isObject()) {
            QJsonObject responseDataObject = responseObject["responseData"].toObject();
            if (responseDataObject.contains("translatedText") && responseDataObject["translatedText"].isString()) {
                QString translatedText = responseDataObject["translatedText"].toString();
                appendText("💬 " + translatedText);

                QString ttsLanguageCode;
                if (langpair == "tr|en") {
                    ttsLanguageCode = "en-US";
                    appendText("🎯 TTS dili: İngilizce");
                } else {
                    ttsLanguageCode = "tr-TR";
                    appendText("🎯 TTS dili: Türkçe");
                }

                qDebug() << "Calling TTS with:" << translatedText << "in" << ttsLanguageCode;
                speakWithGoogleTTS(translatedText, ttsLanguageCode);
            }
        }
    }

    translate_reply.reset();
}

void MainWindow::httpTranslateReadyRead()
{

}

void MainWindow::httpAiFinished()
{
    if(ai_reply->error() != QNetworkReply::NoError)
    {
        qDebug() << "ERROR: AI API:" << ai_reply->errorString();
    }
    ai_reply.reset();
}

void MainWindow::httpGoogleTtsFinished()
{
    if(google_tts_reply->error() != QNetworkReply::NoError)
    {
        qDebug() << "ERROR: Google TTS API:" << google_tts_reply->errorString();
        appendText("❌ TTS Error");
        google_tts_reply.reset();
        return;
    }

    QByteArray responseData = google_tts_reply->readAll();
    QJsonDocument jsonResponse = QJsonDocument::fromJson(responseData);

    if (!jsonResponse.isNull() && jsonResponse.isObject()) {
        QJsonObject responseObject = jsonResponse.object();

        if (responseObject.contains("audioContent")) {
            QString audioContentBase64 = responseObject["audioContent"].toString();
            QByteArray audioData = QByteArray::fromBase64(audioContentBase64.toUtf8());

            QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);

            // ✅ Unique dosya adı (timestamp ile)
            QString timestamp = QString::number(QDateTime::currentMSecsSinceEpoch());
            QString audioFilePath = tempPath + "/google_tts_" + timestamp + ".mp3";

            QFile audioFile(audioFilePath);
            if (audioFile.open(QIODevice::WriteOnly)) {
                audioFile.write(audioData);
                audioFile.close();

                qDebug() << "TTS audio saved:" << audioFilePath << "Size:" << audioData.size();

                playGoogleTTSAudio(audioFilePath);
            } else {
                qDebug() << "ERROR: Failed to save TTS audio:" << audioFile.errorString();
            }
        } else if (responseObject.contains("error")) {
            QJsonObject errorObj = responseObject["error"].toObject();
            QString errorMsg = errorObj["message"].toString();
            qDebug() << "ERROR: Google TTS API:" << errorMsg;
            appendText("❌ TTS: " + errorMsg);
        }
    }

    google_tts_reply.reset();
}

void MainWindow::httpGoogleTtsReadyRead()
{
    // ✅ Boş bırak
}

void MainWindow::speechVoice()
{
    // ✅ Eski reply temizle
    if (speech_reply) {
        speech_reply->disconnect();
        speech_reply.reset();
    }

    QString filePath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QDir folderDir(filePath);
    QStringList files = folderDir.entryList(QDir::Files);
    for (const QString &_file : files) {
        QString file_Path = filePath + QDir::separator() + _file;
        if(file_Path.contains("record."))
        {
            file.setFileName(file_Path);
            break;
        }
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "ERROR: Cannot open file:" << file.errorString();
        return;
    }

    QByteArray fileData = file.readAll();
    file.close();

    QJsonObject config {
        {"encoding", "LINEAR16"},
        {"languageCode", languageCode},
        {"model", "command_and_search"},
        {"enableAutomaticPunctuation", true},
        {"audioChannelCount", QJsonValue::fromVariant(channelCount)},
        {"enableWordTimeOffsets", false},
        {"sampleRateHertz", QJsonValue::fromVariant(sampleRate)}
    };

    QJsonDocument data {
        QJsonObject {
            {"audio", QJsonObject { {"content", QJsonValue::fromVariant(fileData.toBase64())} }},
            {"config", config}
        }
    };

    this->urlSpeech.setQuery("key=" + speechApiKey);
    speech_reply.reset(qnam->post(QNetworkRequest(urlSpeech), data.toJson(QJsonDocument::Compact)));

    connect(speech_reply.get(), &QNetworkReply::sslErrors, this, &MainWindow::sslErrors);
    connect(speech_reply.get(), &QNetworkReply::finished, this, &MainWindow::httpSpeechFinished);
    // ✅ readyRead kaldır
}

void MainWindow::httpAiReadyRead()
{
    auto data = QJsonDocument::fromJson(ai_reply->readAll());
}

void MainWindow::speakWithGoogleTTS(const QString &text, const QString &languageCode)
{
    qDebug() << "=== TTS Request ===";
    qDebug() << "Text:" << text;
    qDebug() << "Language:" << languageCode;

    if (google_tts_reply) {
        google_tts_reply->disconnect();
        google_tts_reply.reset();
    }

    QString voiceName;

    if (languageCode.startsWith("tr")) {
        voiceName = "tr-TR-Wavenet-E";
    } else if (languageCode.startsWith("en")) {
        voiceName = "en-US-Wavenet-F";
    } else {
        voiceName = languageCode + "-Wavenet-A";
    }

    QJsonObject voice {
        {"languageCode", languageCode},
        {"name", voiceName}
    };

    QJsonObject input {
        {"text", text}
    };

    QJsonObject audioConfig {
        {"audioEncoding", "MP3"},
        {"speakingRate", 1.0},
        {"pitch", 0.0},
        {"volumeGainDb", 0.0}
    };

    QJsonDocument requestDoc {
        QJsonObject {
            {"input", input},
            {"voice", voice},
            {"audioConfig", audioConfig}
        }
    };

    this->urlGoogleTts.setQuery("key=" + speechApiKey);

    QNetworkRequest request(urlGoogleTts);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);

    google_tts_reply.reset(qnam->post(request, requestDoc.toJson(QJsonDocument::Compact)));

    connect(google_tts_reply.get(), &QNetworkReply::sslErrors, this, &MainWindow::sslErrors);
    connect(google_tts_reply.get(), &QNetworkReply::finished, this, &MainWindow::httpGoogleTtsFinished);
}

void MainWindow::playGoogleTTSAudio(const QString &filePath)
{
    // ✅ Önce player'ı tamamen durdur
    m_ttsPlayer->stop();
    m_ttsPlayer->setSource(QUrl());

    // ✅ Biraz bekle (player'ın temizlenmesi için)
    QThread::msleep(100);

    // ✅ Yeni dosyayı yükle ve çal
    m_ttsPlayer->setSource(QUrl::fromLocalFile(filePath));
    m_ttsAudioOutput->setVolume(0.6);
    m_ttsPlayer->play();
}

void MainWindow::onTtsPlayerStateChanged(QMediaPlayer::PlaybackState state)
{
    switch(state) {
    case QMediaPlayer::PlayingState:
        m_speaking = true;
        appendText("🔊 Konuşuyor...");
        break;

    case QMediaPlayer::StoppedState:
    {
        // ✅ Scope ekle (süslü parantez)
        m_speaking = false;
        m_lastSpeechEndTime = QDateTime::currentDateTime();
        appendText("🔇 Konuşma bitti");

        // Eski TTS dosyalarını temizle
        QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        QDir tempDir(tempPath);
        QStringList filters;
        filters << "google_tts_*.mp3";
        QFileInfoList oldFiles = tempDir.entryInfoList(filters, QDir::Files);

        // Son dosya hariç diğerlerini sil
        if (oldFiles.size() > 1) {
            for (int i = 0; i < oldFiles.size() - 1; i++) {
                QFile::remove(oldFiles[i].absoluteFilePath());
            }
        }
        break;
    }

    case QMediaPlayer::PausedState:
        break;
    }
}

void MainWindow::translateText(QString text, QString langpair)
{
    // ✅ Eski reply varsa temizle
    if (translate_reply) {
        translate_reply->disconnect();
        translate_reply.reset();
    }

    QUrlQuery query;
    query.addQueryItem("langpair", langpair);
    query.addQueryItem("q", text);
    query.addQueryItem("mt", "1");
    query.addQueryItem("onlyprivate", "0");
    query.addQueryItem("de", "a@b.c");

    // ✅ Timestamp ekle (cache bypass)
    query.addQueryItem("_t", QString::number(QDateTime::currentMSecsSinceEpoch()));

    this->urlLanguageTranslate.setQuery(query);

    QNetworkRequest request(this->urlLanguageTranslate);
    request.setRawHeader("Authorization", "q9Oh7Lg7J0Hd");
    request.setRawHeader("X-RapidAPI-Host", translateHost.toStdString().c_str());
    request.setRawHeader("X-RapidAPI-Key", translateApiKey.toStdString().c_str());

    // ✅ Cache kontrolü ekle
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);

    translate_reply.reset(qnam->get(request));

    connect(translate_reply.get(), &QNetworkReply::sslErrors, this, &MainWindow::sslErrors);
    connect(translate_reply.get(), &QNetworkReply::finished, this, &MainWindow::httpTranslateFinished);
    // ✅ readyRead'i kaldır
}

void MainWindow::getFromAi(QString text)
{
    QUrlQuery query;
    query.addQueryItem("message", text);
    query.addQueryItem("user_id", "420");
    this->urlAi.setQuery(query);

    QNetworkRequest request(this->urlAi);
    request.setRawHeader("Authorization", "q9Oh7Lg7J0Hd");
    request.setRawHeader("X-Rapidapi-Host", aiHost.toStdString().c_str());
    request.setRawHeader("X-Rapidapi-Key", aiApiKey.toStdString().c_str());

    ai_reply.reset(qnam->get(request));

    connect(ai_reply.get(), &QNetworkReply::sslErrors, this, &MainWindow::sslErrors);
    connect(ai_reply.get(), &QNetworkReply::finished, this, &MainWindow::httpAiFinished);
    connect(ai_reply.get(), &QIODevice::readyRead, this, &MainWindow::httpAiReadyRead);
}

void MainWindow::inputDeviceChanged(int index)
{
    const QAudioDevice &inputDevice = ui->audioInputDeviceBox->itemData(index).value<QAudioDevice>();

    qDeleteAll(m_audioLevels);
    m_audioLevels.clear();

    audio_format.setSampleFormat(QAudioFormat::Int16);
    audio_format.setSampleRate(sampleRate);
    audio_format.setChannelCount(channelCount);

    if (m_audioInputSource) {
        m_audioInputSource->stop();
        delete m_audioInputSource;
    }

    m_audioInputSource = new QAudioSource(inputDevice, audio_format);
    m_audioInputSource->setBufferSize(1024);
    m_audioInputSource->setVolume(0.9);

    m_audioInput->setDevice(inputDevice);
    m_audioInput->setVolume(0.9);

    m_captureSession.setAudioInput(m_audioInput);
    m_captureSession.setRecorder(m_audioRecorder);

    m_audioLevels.clear();
    for (int i = 0; i < audio_format.channelCount(); ++i) {
        AudioLevel *level = new AudioLevel(ui->centralwidget);
        level->setMinimumSize(QSize(0,50));
        m_audioLevels.append(level);
        ui->levelsLayout->addWidget(level);
    }

    ioInputDevice = m_audioInputSource->start();
    connect(ioInputDevice, &QIODevice::readyRead, this, &MainWindow::micBufferReady);

    appendText("🎤 " + inputDevice.description());
}

void MainWindow::outputDeviceChanged(int index)
{
    if(m_audioOutput)
    {
        disconnect(m_audioOutput,&QAudioSink::stateChanged, this, &MainWindow::handleAudioStateChanged);
        const QAudioDevice &outputDevice = ui->audioOutputDeviceBox->itemData(index).value<QAudioDevice>();
        if (outputDevice.isFormatSupported(m_format)) {
            m_audioOutput = new QAudioSink(outputDevice, m_format, this);
            connect(m_audioOutput,&QAudioSink::stateChanged, this, &MainWindow::handleAudioStateChanged);
        }
        appendText("🔊 " + outputDevice.description());
    }
}

void MainWindow::updateProgress(qint64 duration)
{
    if (m_audioRecorder->error() != QMediaRecorder::NoError)
        return;

    if (duration >= this->recordDuration &&
        this->m_audioRecorder->recorderState() != QMediaRecorder::RecorderState::StoppedState &&
        this->m_audioRecorder->recorderState() == QMediaRecorder::RecorderState::RecordingState)
    {
        this->m_audioRecorder->stop();
        speechVoice();
    }
}

void MainWindow::onStateChanged(QMediaRecorder::RecorderState state)
{
    QString statusMessage;

    switch (state) {
    case QMediaRecorder::RecordingState:
        ui->recordButton->setText(tr("Stop"));
        break;
    case QMediaRecorder::PausedState:
        ui->recordButton->setText(tr("Stop"));
        clearAudioLevels();
        break;
    case QMediaRecorder::StoppedState:
        clearAudioLevels();
        ui->recordButton->setText(tr("Record"));
        break;
    }
}

void MainWindow::onSpeechStateChanged(QTextToSpeech::State state)
{
    // Fallback Qt TTS - Google TTS kullanıldığında çağrılmaz
}

QMediaFormat MainWindow::selectedMediaFormat() const
{
    QMediaFormat format;
    format.resolveForEncoding(QMediaFormat::NoFlags);
    format.setFileFormat(QMediaFormat::FileFormat::Wave);
    format.setAudioCodec(QMediaFormat::AudioCodec::Wave);
    return format;
}

void MainWindow::appendText(QString text)
{
    ui->textTerminal->append(text);
}

void MainWindow::toggleRecord()
{
    if(!m_recording)
    {
        m_recording = true;
#ifndef __APPLE__
        if (m_audioRecorder->recorderState() == QMediaRecorder::StoppedState)
        {
            setOutputFile();
            auto format = selectedMediaFormat();
            m_audioRecorder->setMediaFormat(format);
            m_audioRecorder->setAudioBitRate(16000);
            m_audioRecorder->setAudioSampleRate(sampleRate);
            m_audioRecorder->setAudioChannelCount(channelCount);
            m_audioRecorder->setQuality(QMediaRecorder::HighQuality);
            m_audioRecorder->setEncodingMode(QMediaRecorder::ConstantQualityEncoding);
            m_audioRecorder->record();
        }
#endif
        this->recordDuration = boxValue(ui->recordTimeBox).toInt();
    }
    else
    {
#ifndef __APPLE__
        m_audioRecorder->stop();
#endif
        m_recording = false;
    }
}

void MainWindow::toggleLanguage()
{
    if(langpair == "tr|en")
    {
        langpair = "en|tr";
        languageCode = "en-US";
        ui->languageButton->setText("EN→TR");
        appendText("🔄 İngilizce → Türkçe");
    }
    else
    {
        langpair = "tr|en";
        languageCode = "tr-TR";
        ui->languageButton->setText("TR→EN");
        appendText("🔄 Türkçe → İngilizce");
    }

    setSpeechEngine();
}

void MainWindow::displayErrorMessage()
{
    qDebug() << "ERROR: Recorder:" << m_audioRecorder->errorString();
    appendText("❌ " + m_audioRecorder->errorString());
    m_recording = false;
}

void MainWindow::clearAudioLevels()
{
    for (auto m_audioLevel : qAsConst(m_audioLevels))
        m_audioLevel->setLevel(0);
}

QList<qreal> MainWindow::getBufferLevels(const QAudioBuffer &buffer)
{
    QList<qreal> values;
    auto format = buffer.format();
    if (!format.isValid())
        return values;
    int channels = buffer.format().channelCount();
    values.fill(0, channels);
    int bytesPerSample = format.bytesPerSample();
    QList<qreal> max_values;
    max_values.fill(0, channels);
    const char *data = buffer.constData<char>();

    for (int i = 0; i < buffer.frameCount(); ++i) {
        for (int j = 0; j < channels; ++j) {
            qreal value = qAbs(format.normalizedSampleValue(data));
            qreal amplifiedValue = value * 10;

            if (amplifiedValue >= m_vox_sensitivity)
            {
                // ✅ CRITICAL: TTS çalıyorsa KESİNLİKLE kayıt başlatma
                if (m_speaking) {
                    data += bytesPerSample;
                    continue;
                }

                // ✅ Zaten kaydediyorsa tekrar başlatma
                if (m_recording) {
                    data += bytesPerSample;
                    continue;
                }

                // ✅ Cooldown kontrolü
                qint64 timeSinceSpeech = m_lastSpeechEndTime.msecsTo(QDateTime::currentDateTime());
                bool cooldownPassed = timeSinceSpeech > SPEECH_COOLDOWN_MS;

                if (!cooldownPassed) {
                    data += bytesPerSample;
                    continue;
                }

                // ✅ Sadece buraya gelirse kayda başla
                toggleRecord();
            }

            if (value > max_values.at(j))
                max_values[j] = value;
            data += bytesPerSample;
        }
    }
    return max_values;
}

void MainWindow::micBufferReady()
{
    qint64 bytesAvailable = m_audioInputSource->bytesAvailable();
    if (bytesAvailable <= 0) {
        return;
    }
    auto buffer = ioInputDevice->read(bytesAvailable);
    const QAudioBuffer &audioBuffer = QAudioBuffer(buffer, audio_format);
    processBuffer(audioBuffer);
#ifdef __APPLE__
    recordBuffer(buffer);
#endif
}

void MainWindow::processBuffer(const QAudioBuffer& buffer)
{
    QList<qreal> levels = getBufferLevels(buffer);
    for (int i = 0; i < levels.count(); ++i)
        m_audioLevels.at(i)->setLevel(levels.at(i));
}

#ifdef __APPLE__
void MainWindow::startRecording()
{
    QString filePath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/record.wav";
    QByteArray audioFilePathBytes = filePath.toLocal8Bit();
    const char *audioFilePath = audioFilePathBytes.constData();

    fileURL = CFURLCreateFromFileSystemRepresentation(NULL,
                                                      (UInt8*)audioFilePath,
                                                      strlen(audioFilePath),
                                                      false);

    if (!fileURL) {
        qDebug() << "ERROR: Couldn't create URL for audio file";
        return;
    }

    AudioStreamBasicDescription audioFormat;
    audioFormat.mSampleRate = sampleRate;
    audioFormat.mFormatID = kAudioFormatLinearPCM;
    audioFormat.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
    audioFormat.mBytesPerPacket = sizeof(short) * channelCount;
    audioFormat.mFramesPerPacket = 1;
    audioFormat.mBytesPerFrame = sizeof(short) * channelCount;
    audioFormat.mChannelsPerFrame = channelCount;
    audioFormat.mBitsPerChannel = 8 * sizeof(short);

    OSStatus status = AudioFileCreateWithURL((CFURLRef)fileURL,
                                             kAudioFileWAVEType,
                                             &audioFormat,
                                             kAudioFileFlags_EraseFile,
                                             &audioFile);

    if (status != noErr)
    {
        qDebug() << "ERROR: Creating audio file, code:" << status;
        return;
    }

    CFRelease(fileURL);
}

void MainWindow::stopRecording()
{
    if (audioFile != nullptr) {
        AudioFileClose(audioFile);
        audioFile = nullptr;
        speechVoice();
    }
}

void MainWindow::writeAudioToFile(const QByteArray& buffer)
{
    const short* audioData = reinterpret_cast<const short*>(buffer.constData());
    UInt32 numBytesToWrite = buffer.size();
    off_t offset = 0;

    OSStatus status = AudioFileWriteBytes(audioFile,
                                          false,
                                          offset,
                                          &numBytesToWrite,
                                          audioData);

    if (status != noErr)
    {
        qDebug() << "ERROR: Writing audio file, code:" << status;
    }
}

bool MainWindow::isBufferFull() const
{
    qint64 bufferSize = accumulatedBuffer.size() / (sizeof(short) * channelCount);
    qint64 durationMilliseconds = (bufferSize * 1000) / sampleRate;
    return durationMilliseconds >= this->recordDuration;
}

void MainWindow::recordBuffer(const QByteArray& buffer)
{
    if (m_recording)
    {
        if (!m_record_started)
        {
            startRecording();
            m_record_started = true;
            ui->recordButton->setText(tr("Stop"));
        }

        accumulatedBuffer.append(buffer);

        if (isBufferFull()) {
            m_recording = false;
        }
    }
    else if (m_record_started)
    {
        writeAudioToFile(accumulatedBuffer);
        accumulatedBuffer.clear();
        stopRecording();
        m_recording = false;
        m_record_started = false;
        ui->recordButton->setText(tr("Record"));
    }
}
#endif

void MainWindow::on_exitButton_clicked()
{
    QApplication::quit();
}

void MainWindow::on_clearButton_clicked()
{
    ui->textTerminal->clear();
}

void MainWindow::on_recordButton_clicked()
{
    toggleRecord();
}

void MainWindow::on_micVolumeSlider_valueChanged(int value)
{
    qreal linearVolume = QAudio::convertVolume(value / qreal(100),
                                               QAudio::LogarithmicVolumeScale,
                                               QAudio::LinearVolumeScale);
    m_captureSession.audioInput()->setVolume(linearVolume);
    m_audioInputSource->setVolume(linearVolume);
    ui->labelMicLevelInfo->setText(QString::number(m_captureSession.audioInput()->volume() * 100, 'f', 0) + "%");
}

void MainWindow::on_speechVolumeSlider_valueChanged(int value)
{
    qreal linearVolume = QAudio::convertVolume(value / qreal(100),
                                               QAudio::LogarithmicVolumeScale,
                                               QAudio::LinearVolumeScale);
    m_ttsAudioOutput->setVolume(linearVolume);
    ui->labelSpeechLevelInfo->setText(QString::number(linearVolume * 100, 'f', 0) + "%");
}

void MainWindow::on_voxSensivitySlider_valueChanged(int value)
{
    qreal linearVox = QAudio::convertVolume(value / qreal(100),
                                            QAudio::LogarithmicVolumeScale,
                                            QAudio::LinearVolumeScale);
    m_vox_sensitivity = linearVox;
    ui->labelVoxSensivityInfo->setText(QString::number(linearVox, 'f', 2));
}

void MainWindow::on_languageButton_clicked()
{
    toggleLanguage();
}
