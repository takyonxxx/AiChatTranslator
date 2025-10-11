#ifndef CLAUDEAI_H
#define CLAUDEAI_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

class ClaudeAI : public QObject
{
    Q_OBJECT

public:
    explicit ClaudeAI(const QString &apiKey, QObject *parent = nullptr);
    ~ClaudeAI();

    // Ana fonksiyon - Claude'a soru sor
    void askClaude(const QString &text, int maxWords = 100);

    // API key değiştir
    void setApiKey(const QString &apiKey);

    // Model değiştir (default: claude-sonnet-4-5)
    void setModel(const QString &model);

signals:
    // Cevap geldiğinde
    void responseReceived(const QString &response);

    // Hata olduğunda
    void errorOccurred(const QString &error);

private slots:
    void onReplyFinished();

private:
    QNetworkAccessManager *m_networkManager;
    QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> m_reply;
    QString m_apiKey;
    QString m_model;

    const QString API_URL = "https://api.anthropic.com/v1/messages";
    const QString API_VERSION = "2023-06-01";
};

#endif // CLAUDEAI_H
