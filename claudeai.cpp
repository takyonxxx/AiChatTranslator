#include "claudeai.h"
#include <QDebug>

ClaudeAI::ClaudeAI(const QString &apiKey, QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_apiKey(apiKey)
    , m_model("claude-sonnet-4-5-20250929")  // En son model
{
}

ClaudeAI::~ClaudeAI()
{
    delete m_networkManager;
}

void ClaudeAI::setApiKey(const QString &apiKey)
{
    m_apiKey = apiKey;
}

void ClaudeAI::setModel(const QString &model)
{
    m_model = model;
}

void ClaudeAI::askClaude(const QString &text, int maxWords)
{
    if (m_apiKey.isEmpty()) {
        emit errorOccurred("API key boş!");
            return;
    }

    if (text.isEmpty()) {
        emit errorOccurred("Soru metni boş!");
            return;
    }

    // Eski reply varsa temizle
    if (m_reply) {
        m_reply->disconnect();
        m_reply.reset();
    }

    // Request hazırla
    QNetworkRequest request(API_URL);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("x-api-key", m_apiKey.toUtf8());
    request.setRawHeader("anthropic-version", API_VERSION.toUtf8());

    // System prompt ile max kelime sınırı belirt
    QString systemPrompt = QString(
                               "You are a helpful assistant. "
                               "Always respond in the same language as the user's question (Turkish or English). "
                               "Keep your response concise and limit it to approximately %1 words. "
                               "Be direct and helpful."
                               ).arg(maxWords);

    // JSON body oluştur
    QJsonObject messageContent;
    messageContent["type"] = "text";
    messageContent["text"] = text;

    QJsonArray messages;
    QJsonObject userMessage;
    userMessage["role"] = "user";
    userMessage["content"] = QJsonArray{messageContent};
    messages.append(userMessage);

    QJsonObject requestBody;
    requestBody["model"] = m_model;
    requestBody["max_tokens"] = 1024;
    requestBody["system"] = systemPrompt;
    requestBody["messages"] = messages;

    QJsonDocument doc(requestBody);
    QByteArray data = doc.toJson(QJsonDocument::Compact);

    qDebug() << "Sending request to Claude API...";
    qDebug() << "Question:" << text;
    qDebug() << "Max words:" << maxWords;

    // Request gönder
    m_reply.reset(m_networkManager->post(request, data));
    connect(m_reply.get(), &QNetworkReply::finished, this, &ClaudeAI::onReplyFinished);
}

void ClaudeAI::onReplyFinished()
{
    if (!m_reply) {
        return;
    }

    // Hata kontrolü
    if (m_reply->error() != QNetworkReply::NoError) {
        QString errorMsg = QString("Network error: %1").arg(m_reply->errorString());
        qDebug() << "ERROR:" << errorMsg;
        emit errorOccurred(errorMsg);
        m_reply.reset();
        return;
    }

    // Response oku
    QByteArray responseData = m_reply->readAll();
    QJsonDocument jsonResponse = QJsonDocument::fromJson(responseData);

    if (jsonResponse.isNull() || !jsonResponse.isObject()) {
        emit errorOccurred("Invalid JSON response");
        m_reply.reset();
        return;
    }

    QJsonObject responseObject = jsonResponse.object();

    // Hata kontrolü
    if (responseObject.contains("error")) {
        QJsonObject errorObj = responseObject["error"].toObject();
        QString errorMsg = errorObj["message"].toString();
        qDebug() << "ERROR: Claude API:" << errorMsg;
        emit errorOccurred("Claude API: " + errorMsg);
        m_reply.reset();
        return;
    }

    // Response parse et
    if (responseObject.contains("content")) {
        QJsonArray contentArray = responseObject["content"].toArray();

        if (!contentArray.isEmpty()) {
            QJsonObject contentObj = contentArray[0].toObject();
            QString responseText = contentObj["text"].toString();

            qDebug() << "Response received:" << responseText;
            emit responseReceived(responseText);
        } else {
            emit errorOccurred("Empty response from Claude");
        }
    } else {
        emit errorOccurred("Invalid response format");
    }

    m_reply.reset();
}
