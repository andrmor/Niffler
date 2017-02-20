#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <QVector>
#include <QJsonObject>
#include <QJsonArray>

bool parseJson(const QJsonObject &json, const QString &key, bool &var);
bool parseJson(const QJsonObject &json, const QString &key, int &var);
bool parseJson(const QJsonObject &json, const QString &key, unsigned int &var);
bool parseJson(const QJsonObject &json, const QString &key, double &var);
bool parseJson(const QJsonObject &json, const QString &key, float &var);
bool parseJson(const QJsonObject &json, const QString &key, QString &var);
bool parseJson(const QJsonObject &json, const QString &key, QJsonArray &var);

bool LoadJsonFromFile(QJsonObject &json, QString fileName);
bool SaveJsonToFile(QJsonObject &json, QString fileName);

bool LoadFloatVectorsFromFile(QString FileName, QVector<float>* x, QVector<float>* y, QVector<float>* z);
const QString JsonToString(QJsonObject &json);
const QJsonObject StringToJson(const QString string);

// Find this comment, bro.

#endif // UTILS_H
