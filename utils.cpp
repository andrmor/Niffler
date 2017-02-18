#include "utils.h"

#include <limits>

#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QFileDialog>
#include <QDebug>

bool parseJson(const QJsonObject &json, const QString &key, bool &var)
{
  if (json.contains(key))
    {
      var = json[key].toBool();
      return true;
    }
  else return false;
}
bool parseJson(const QJsonObject &json, const QString &key, int &var)
{
  if (json.contains(key))
    {
      var = json[key].toInt();
      return true;
    }
  else return false;
}
bool parseJson(const QJsonObject &json, const QString &key, unsigned int &var)
{
  if (json.contains(key))
    {
      unsigned int tmp = json[key].toInt();
      if (tmp>=0 && tmp<std::numeric_limits<unsigned int>::max())
      {
          var = tmp;
          return true;
      }
    }
  return false;
}
bool parseJson(const QJsonObject &json, const QString &key, double &var)
{
  if (json.contains(key))
    {
      var = json[key].toDouble();
      return true;
    }
  else return false;
}
bool parseJson(const QJsonObject &json, const QString &key, float &var)
{
  if (json.contains(key))
    {
      var = json[key].toDouble();
      return true;
    }
  else return false;
}
bool parseJson(const QJsonObject &json, const QString &key, QString &var)
{
  if (json.contains(key))
    {
      var = json[key].toString();
      return true;
    }
  else return false;
}

bool parseJson(const QJsonObject &json, const QString &key, QJsonArray &var)
{
  if (json.contains(key))
    {
      var = json[key].toArray();
      return true;
    }
  else return false;
}

bool LoadJsonFromFile(QJsonObject &json, QString fileName)
{
    QFile loadFile(fileName);
    if (loadFile.open(QIODevice::ReadOnly))
    {
        QByteArray saveData = loadFile.readAll();
        QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
        json = loadDoc.object();
        loadFile.close();
        return true;
    }
    qWarning() << "Cannot open file: "+fileName;
    return false;
}

bool SaveJsonToFile(QJsonObject &json, QString fileName)
{
    QJsonDocument saveDoc(json);
    QFile saveFile(fileName);
    if (saveFile.open(QIODevice::WriteOnly))
    {
        saveFile.write(saveDoc.toJson());
        saveFile.close();
        return true;
    }
    qWarning() << "Couldn't save json to file: "+fileName;
    return false;
}

bool LoadFloatVectorsFromFile(QString FileName, QVector<float>* x, QVector<float>* y, QVector<float>* z)
{
    if (FileName.isEmpty()) return false;

    QFile file(FileName);
    if(!file.open(QIODevice::ReadOnly | QFile::Text)) return false;

    QTextStream in(&file);
    QRegExp rx("(\\ |\\,|\\:|\\t)"); //separators: ' ' or ',' or ':' or '\t'
    x->resize(0);
    y->resize(0);
    z->resize(0);
    while(!in.atEnd())
    {
        QString line = in.readLine();
        QStringList fields = line.split(rx, QString::SkipEmptyParts);

        bool ok1=false, ok2, ok3;
        double xx, yy, zz;
        if (fields.size() > 2 )
        {
            xx = fields[0].toDouble(&ok1);  //*** potential problem with decimal separator!
            yy = fields[1].toDouble(&ok2);
            zz = fields[2].toDouble(&ok3);
        }
        if (ok1 && ok2 && ok3)
        {
            x->append(xx);
            y->append(yy);
            z->append(zz);
        }
    }
    file.close();

    if (x->isEmpty()) return false;
    return true;
}

const QString JsonToString(QJsonObject &json)
{
    QJsonDocument doc(json);
    QString str(doc.toJson(QJsonDocument::Compact));
    return str;
}

const QJsonObject StringToJson(const QString string)
{
    QJsonDocument doc = QJsonDocument::fromJson(string.toUtf8());

    if(!doc.isNull())
        if(doc.isObject())
            return doc.object();

    qDebug() << "String is not a QJsonObject!";
    return QJsonObject();
}
