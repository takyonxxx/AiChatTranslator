#ifndef TRANSLATECLIENT_H
#define TRANSLATECLIENT_H
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QDebug>
#include "constants.h"

class TranslateClient : public QObject {
    Q_OBJECT

public:
    TranslateClient(QObject* parent = nullptr) : QObject(parent) {}

    QString translateText(QString text, QString langpair) {
        QUrl apiUrl;
        apiUrl.setUrl(translateUrl);

        QUrlQuery query;
        query.addQueryItem("langpair", langpair);
        query.addQueryItem("q", text);
        query.addQueryItem("mt", "1");
        query.addQueryItem("onlyprivate", "0");
        query.addQueryItem("de", "a@b.c");

        apiUrl.setQuery(query);

        QNetworkRequest request(apiUrl);
        request.setRawHeader("Authorization", "q9Oh7Lg7J0Hd");
        request.setRawHeader("X-RapidAPI-Host", translateHost.toStdString().c_str());
        request.setRawHeader("X-RapidAPI-Key", translateApiKey.toStdString().c_str());

        QNetworkAccessManager manager;
        QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> reply;
        reply.reset(manager.get(request));
        QSharedPointer<QEventLoop> loop = QSharedPointer<QEventLoop>::create();
        connect(reply.get(), &QNetworkReply::finished, loop.data(), &QEventLoop::quit);
        loop->exec();

        // Check if the request was successful
        if (reply->error() == QNetworkReply::NoError) {
            QString strReply = reply->readAll().toStdString().c_str();
            QJsonDocument jsonResponseDoc = QJsonDocument::fromJson(strReply.toUtf8());

            if (!jsonResponseDoc.isNull() && jsonResponseDoc.isObject()) {
                QJsonObject responseObject = jsonResponseDoc.object();
                if (responseObject.contains("responseData") && responseObject["responseData"].isObject()) {
                    QJsonObject responseDataObject = responseObject["responseData"].toObject();
                    if (responseDataObject.contains("translatedText") && responseDataObject["translatedText"].isString()) {
                        QString translatedText = responseDataObject["translatedText"].toString();
                        return translatedText;
                    } else {
                        qDebug() << "translatedText not found in responseData";
                        // Handle the absence of translatedText gracefully
                        return "Translation error: Text not available";
                    }
                } else {
                    qDebug() << "responseData not found in the response";
                    // Handle the absence of responseData gracefully
                    return "Translation error: Response data not available";
                }
            }
        } else {
            // Error occurred, handle it using the stored response data
            QString errorMessage = reply->errorString();
            qDebug() << errorMessage;
            return errorMessage;
        }

        return QString();
    }
};

#endif // TRANSLATECLIENT_H
