#ifndef GPT3CLIENT_H
#define GPT3CLIENT_H
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include "constants.h"

class GPT3Client : public QObject {
    Q_OBJECT

public:
    GPT3Client(QObject* parent = nullptr) : QObject(parent) {}

    QStringList getWordsFromChatGptNodel(QString prompt) {
        QString apiKey = gpt3ApiKey;
        QUrl apiUrl;
        apiUrl.setUrl(gpt3BaseApi);
        QNetworkRequest request(apiUrl);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("Authorization", "Bearer " + apiKey.toUtf8());

        // Construct the JSON request payload
        QJsonObject requestData;
        requestData["prompt"] = prompt;
        requestData["max_tokens"] = 10; // Adjust the number of tokens based on your needs

        QJsonDocument requestDoc(requestData);
        QByteArray requestDataBytes = requestDoc.toJson();

        // Perform the API request synchronously (for simplicity)
        QNetworkAccessManager manager;
        QNetworkReply* reply = manager.post(request, requestDataBytes);
        QEventLoop loop;
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        // Process the API response
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);
            QJsonObject responseObject = responseDoc.object();
            QJsonArray choices = responseObject["choices"].toArray();
            qDebug() << "API Response:" << responseData;

            QStringList newWords;
            for (const auto& choice : choices) {
                QString word = choice.toObject()["text"].toString();
                newWords << word.trimmed();
            }

            return newWords;
        } else {
            qDebug() << "Error fetching new words:" << reply->errorString();
            qDebug() << "API Response:" << reply->readAll();
            return QStringList();
        }
    }

    QString getAnswerFromChatGpt(QString prompt) {
        QString apiKey = gpt3ApiKey;
        QUrl apiUrl;
        apiUrl.setUrl(gpt3BaseApi);
        QNetworkRequest request(apiUrl);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("Authorization", "Bearer " + apiKey.toUtf8());

        // Construct the JSON request payload
        QJsonObject requestData;
        requestData["prompt"] = prompt;
        requestData["max_tokens"] = 10; // Adjust the number of tokens based on your needs

        QJsonDocument requestDoc(requestData);
        QByteArray requestDataBytes = requestDoc.toJson();

        // Perform the API request synchronously (for simplicity)
        QNetworkAccessManager manager;
        QNetworkReply* reply = manager.post(request, requestDataBytes);
        QEventLoop loop;
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        // Check if the request was successful
        if (reply->error() == QNetworkReply::NoError) {
            // Read the response data regardless of the error status
            QByteArray responseData = reply->readAll();
            QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);

            if (responseDoc.isObject()) {
                QJsonObject responseObject = responseDoc.object();
                QString jsonString = QString::fromUtf8(responseDoc.toJson());
                QJsonArray choices = responseObject["choices"].toArray();
                qDebug() << "API Response:" << responseData;

                QStringList newWords;
                for (const auto& choice : choices) {
                    QString word = choice.toObject()["text"].toString();
                    qDebug() << word;
                }

                return jsonString;
            } else {
                qDebug() << "Invalid JSON response:" << responseData;
                return "Invalid JSON response";
            }
        } else {
            // Error occurred, handle it using the stored response data
            QString errorMessage = reply->errorString();
            qDebug() << errorMessage;
            return errorMessage;
        }
    }
};

#endif // GPT3CLIENT_H
