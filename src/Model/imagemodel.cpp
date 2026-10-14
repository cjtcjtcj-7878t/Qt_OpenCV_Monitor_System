#include "imagemodel.h"

#include "../util/mysqlconnect.h"

#include <QDir>
#include <QDirIterator>
#include <QSet>
#include <QFileInfo>
#include <QHash>
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>

#include <algorithm>
#include <limits>

namespace
{
struct video_reference
{
    int id = 0;
    int camera_id = 0;
    QString name;
    QString path;
    QString table;
    QDateTime start_time;
    QDateTime end_time;
};

QString pathKey(const QString &path)
{
    return QDir::cleanPath(QFileInfo(path).absoluteFilePath()).toCaseFolded();
}

QString existingVideoPath(const QString &databasePath,const QString &videoName,const QString &videoDirectory)
{
    QString candidate=databasePath.trimmed();
    if(!candidate.isEmpty()&&!QDir::isAbsolutePath(candidate)){
        candidate=QDir(videoDirectory).filePath(candidate);
    }
    candidate=QDir::cleanPath(QFileInfo(candidate).absoluteFilePath());
    if(QFileInfo(candidate).isFile()){
        return candidate;
    }

    // 设置目录改变后数据库可能仍保存旧路径，此时按文件名在新目录中查找。
    const QString localPath=QDir(videoDirectory).filePath(videoName);
    return QFileInfo(localPath).isFile()?QDir::cleanPath(QFileInfo(localPath).absoluteFilePath()):QString();
}

int cameraIdFromImageName(const QString &fileName)
{
    static const QRegularExpression pattern(
        "^(?:detect_)?camera_(\\d+)_",
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match=pattern.match(fileName);
    return match.hasMatch()?match.captured(1).toInt():0;
}

QDateTime timeFromFileName(const QString &fileName)
{
    static const QRegularExpression pattern(
        "(\\d{8})_(\\d{6})_(\\d{3})");
    const QRegularExpressionMatch match=pattern.match(fileName);
    if(!match.hasMatch()){
        return QDateTime();
    }
    return QDateTime::fromString(QString("%1_%2_%3").arg(match.captured(1),match.captured(2),match.captured(3)),"yyyyMMdd_hhmmss_zzz");
}

bool parseVideoFileName(const QString &fileName,int &cameraId,bool &isDetection,QDateTime &startTime)
{
    // 录制文件名由监控线程生成，例如 camera_1_20260915_193704_399.avi。
    // 数据库暂时不可用时，图片回看仍可以用这个公共命名规则找到视频。
    static const QRegularExpression pattern(
        "^(detect_)?camera_(\\d+)_(\\d{8}_\\d{6}_\\d{3})\\.[^.]+$",
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match=pattern.match(fileName);
    if(!match.hasMatch()){
        return false;
    }

    cameraId=match.captured(2).toInt();
    isDetection=!match.captured(1).isEmpty();
    startTime=QDateTime::fromString(match.captured(3),
                                      "yyyyMMdd_hhmmss_zzz");
    return cameraId>0&&startTime.isValid();
}

void appendLocalVideoReferences(const QString &videoDirectory,QList<video_reference> &normalVideos,QList<video_reference> &detectVideos)
{
    QDir directory(videoDirectory.trimmed());
    if(videoDirectory.trimmed().isEmpty()||!directory.exists()){
        return;
    }

    const QStringList filters={
                                  "*.avi","*.AVI","*.mp4","*.MP4","*.mkv","*.MKV",
                                  "*.mov","*.MOV"};
    const QFileInfoList files=directory.entryInfoList(
        filters,QDir::Files|QDir::Readable,QDir::Time);
    for(const QFileInfo &fileInfo:files){
        int cameraId=0;
        bool isDetection=false;
        QDateTime startTime;
        if(!parseVideoFileName(fileInfo.fileName(),cameraId,isDetection,startTime)){
            continue;
        }

        video_reference record;
        record.camera_id=cameraId;
        record.name=fileInfo.fileName();
        record.path=QDir::cleanPath(fileInfo.absoluteFilePath());
        record.table=isDetection?"detect_t":"video_t";
        record.start_time=startTime;
        // 本地兜底只需要匹配同一录制片段的起始时间；数据库记录存在时，
        // 数据库中的 start_time/end_time 仍然优先提供更准确的范围。
        record.end_time=startTime;
        (isDetection?detectVideos:normalVideos).append(record);
    }
}

bool isAutomaticImageName(const QString &fileName)
{
    return fileName.startsWith("detect_camera_",Qt::CaseInsensitive)
    &&fileName.contains("_event_",Qt::CaseInsensitive);
}

const video_reference *findExactVideo(
    const QList<video_reference> &records,
    int videoId,
    int cameraId)
{
    if (videoId <= 0) return nullptr;
    for(const video_reference &record:records){
        if(record.id==videoId
            &&(cameraId<=0||record.camera_id==cameraId)
            &&!record.path.isEmpty()){
            return &record;
        }
    }
    return nullptr;
}

const video_reference *findVideoAtTime(
    const QList<video_reference> &records,
    int cameraId,
    const QDateTime &imageTime)
{
    if(cameraId<=0||!imageTime.isValid()){
        return nullptr;
    }

    const video_reference *best=nullptr;
    qint64 bestDistance=std::numeric_limits<qint64>::max();
    for(const video_reference &record:records){
        if(record.camera_id!=cameraId||record.path.isEmpty()
            ||!record.start_time.isValid()){
            continue;
        }
        const QDateTime end=record.end_time.isValid()
                                  ?record.end_time
                                  :record.start_time;
        // 允许两秒误差，兼容数据库只保存到秒而图片文件名保存到毫秒。
        if(imageTime<record.start_time.addSecs(-2)
            ||imageTime>end.addSecs(2)){
            continue;
        }
        const qint64 distance=qAbs(record.start_time.msecsTo(imageTime));
        if(distance<bestDistance){
            best=&record;
            bestDistance=distance;
        }
    }
    return best;
}

void applyVideoReference(imageinfo &image,const video_reference *record)
{
    if(!record){
        return;
    }
    image.setRelatedVideoPath(record->path);
    image.setRelatedVideoName(record->name);
    image.setVideoTable(record->table);
    image.setRelatedVideoStart(record->start_time);
    image.setRelatedVideoComplete(record->end_time.isValid()
                                  && record->end_time > record->start_time);
}
}

bool imagemodel::insertImage(imageinfo &image,QString *errorMessage)
{
    if(errorMessage){
        errorMessage->clear();
    }

    QSqlDatabase db=mySqlconnect::getOne_instance()->getDb();
    if(!db.open()){
        if(errorMessage){
            *errorMessage=db.lastError().text();
        }
        return false;
    }

    QSqlQuery query(db);
    query.prepare("insert into image_t(Camera_id,Video_id,Imagename,Image_size,"
                  "Image_path,Create_time,event_type) "
                  "values(:camera_id,:video_id,:name,:size,:path,:create_time,"
                  ":event_type)");
    query.bindValue(":camera_id",image.cameraId());
    query.bindValue(":video_id",image.videoId());
    query.bindValue(":name",image.imageName());
    query.bindValue(":size",image.imageSize());
    query.bindValue(":path",image.imagePath());
    query.bindValue(":create_time",image.createTime());
    query.bindValue(":event_type",image.eventType());

    if(query.exec()&&query.numRowsAffected()>0){
        image.setImageId(query.lastInsertId().toInt());
        return true;
    }

    if(errorMessage){
        *errorMessage=query.lastError().text();
    }
    return false;
}

bool imagemodel::queryImages(int channelId,
                             const QDate &date,
                             int limit,
                             int offset,
                             const QString &imageDirectory,
                             const QString &videoDirectory,
                             QList<imageinfo> &images,
                             int &total,QString *errorMessage,
                             int hour,
                             int eventType) const
{
    images.clear();
    total=0;
    if(errorMessage){
        errorMessage->clear();
    }

    const QString imageRoot=imageDirectory.trimmed();
    const QString videoRoot=videoDirectory.trimmed();
    QDir directory(imageRoot);
    if(imageRoot.isEmpty()||!directory.exists()){
        return true;
    }

    QHash<QString,imageinfo> recordsByPath;
    QHash<QString,imageinfo> recordsByName;
    QHash<int,int> channelByCamera;
    QList<video_reference> normalVideos;
    QList<video_reference> detectVideos;

    QSqlDatabase db=mySqlconnect::getOne_instance()->getDb();
    if(db.isValid()&&db.open()){
        QSqlQuery cameraQuery(db);
        if(cameraQuery.exec("select Id,Channel_id from camera_t")){
            while(cameraQuery.next()){
                channelByCamera.insert(cameraQuery.value("Id").toInt(),
                                       cameraQuery.value("Channel_id").toInt());
            }
        }

        QSqlQuery imageQuery(db);
        if(imageQuery.exec(
                "select i.Id,i.Camera_id,i.Video_id,i.Imagename,i.Image_size,"
                "i.Image_path,i.Create_time,i.event_type,c.Channel_id "
                "from image_t i left join camera_t c on c.Id=i.Camera_id")){
            while(imageQuery.next()){
                imageinfo record;
                record.setImageId(imageQuery.value("Id").toInt());
                record.setCameraId(imageQuery.value("Camera_id").toInt());
                record.setVideoId(imageQuery.value("Video_id").toInt());
                record.setImageName(imageQuery.value("Imagename").toString());
                record.setImageSize(imageQuery.value("Image_size").toLongLong());
                record.setImagePath(imageQuery.value("Image_path").toString());
                record.setCreateTime(imageQuery.value("Create_time").toDateTime());
                record.setEventType(imageQuery.value("event_type").toInt());
                record.setChannelId(imageQuery.value("Channel_id").toInt());
                if(!record.imagePath().trimmed().isEmpty()){
                    recordsByPath.insert(pathKey(record.imagePath()),record);
                }
                if(!record.imageName().trimmed().isEmpty()
                    &&!recordsByName.contains(record.imageName().toCaseFolded())){
                    recordsByName.insert(record.imageName().toCaseFolded(),record);
                }
            }
        }else if(errorMessage){
            *errorMessage=imageQuery.lastError().text();
        }

        auto readVideos=[&](const QString &table,
                              QList<video_reference> &target){
            QSqlQuery query(db);
            const QString sql=QString(
                                    "select Id,Camera_id,Videoname,Video_path,start_time,end_time "
                                    "from %1").arg(table);
            if(!query.exec(sql)){
                return;
            }
            while(query.next()){
                video_reference record;
                record.id=query.value("Id").toInt();
                record.camera_id=query.value("Camera_id").toInt();
                record.name=query.value("Videoname").toString();
                record.path=existingVideoPath(query.value("Video_path").toString(),
                                                record.name,
                                                videoRoot);
                record.table=table;
                record.start_time=query.value("start_time").toDateTime();
                const QDateTime preciseStart=timeFromFileName(record.name);
                if(preciseStart.isValid()) record.start_time=preciseStart;
                record.end_time=query.value("end_time").toDateTime();
                target.append(record);
            }
        };
        readVideos("video_t",normalVideos);
        readVideos("detect_t",detectVideos);
    }else if(errorMessage){
        *errorMessage=db.lastError().text();
    }

    // 即使数据库连接失败，也可以根据本地文件名完成基本的图片-视频关联。
    // 数据库记录先加入列表，因此同一视频同时存在时仍优先使用数据库信息。
    appendLocalVideoReferences(videoRoot,normalVideos,detectVideos);

    QList<imageinfo> filtered;
    const QStringList filters={
                                  "*.png","*.PNG","*.jpg","*.JPG","*.jpeg","*.JPEG",
                                  "*.bmp","*.BMP"};
    QFileInfoList files;
    QSet<QString> seen;
    QDirIterator iterator(imageRoot, filters, QDir::Files | QDir::Readable,
                          QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        iterator.next();
        const QFileInfo info = iterator.fileInfo();
        const QString key = pathKey(info.absoluteFilePath());
        if (!seen.contains(key)) { files.append(info); seen.insert(key); }
    }
    // 修改保存目录后，数据库中仍然存在的历史图片也可以查看。
    for (auto it = recordsByPath.cbegin(); it != recordsByPath.cend(); ++it) {
        const QFileInfo info(it.value().imagePath());
        const QString key = pathKey(info.absoluteFilePath());
        if (info.isFile() && !seen.contains(key)) { files.append(info); seen.insert(key); }
    }

    for(const QFileInfo &fileInfo:files){
        const QString actualPath=QDir::cleanPath(fileInfo.absoluteFilePath());
        const QString nameKey=fileInfo.fileName().toCaseFolded();
        imageinfo record;
        if(recordsByPath.contains(pathKey(actualPath))){
            record=recordsByPath.value(pathKey(actualPath));
        }else if(recordsByName.contains(nameKey)
                   && !QFileInfo(recordsByName.value(nameKey).imagePath()).isFile()){
            record=recordsByName.value(nameKey);
        }else{
            record.setImageName(fileInfo.fileName());
            record.setCameraId(cameraIdFromImageName(fileInfo.fileName()));
            record.setEventType(isAutomaticImageName(fileInfo.fileName())?1:0);
            QDateTime fileTime=timeFromFileName(fileInfo.fileName());
            record.setCreateTime(fileTime.isValid()?fileTime:fileInfo.lastModified());
        }

        const QDateTime preciseTime = timeFromFileName(fileInfo.fileName());
        if (preciseTime.isValid()) record.setCreateTime(preciseTime);

        // 始终使用当前目录中真实存在的文件路径，数据库中的旧路径只用于匹配。
        record.setImagePath(actualPath);
        record.setImageName(fileInfo.fileName());
        record.setImageSize(fileInfo.size());
        if(!record.createTime().isValid()){
            record.setCreateTime(fileInfo.lastModified());
        }
        if(record.channelId()<=0){
            record.setChannelId(channelByCamera.value(record.cameraId(),0));
        }

        // event_type=1 是自动侦测截图，优先关联 detect_t；手动截图优先关联 video_t。
        // 旧数据可能来自另一种模式，所以首选文件不存在时再尝试另一张表。
        const bool preferDetection=
            record.eventType()==1
            ||record.imageName().startsWith("detect_",Qt::CaseInsensitive);
        const QList<video_reference> &preferred=
            preferDetection?detectVideos:normalVideos;
        const QList<video_reference> &fallback=
            preferDetection?normalVideos:detectVideos;
        const video_reference *videoRecord=findExactVideo(
            preferred,record.videoId(),record.cameraId());
        if(!videoRecord){
            videoRecord=findVideoAtTime(preferred,
                                          record.cameraId(),
                                          record.createTime());
        }
        if(!videoRecord){
            videoRecord=findVideoAtTime(fallback,
                                          record.cameraId(),
                                          record.createTime());
        }
        applyVideoReference(record,videoRecord);

        if(date.isValid()&&record.createTime().isValid()
            &&record.createTime().date()!=date){
            continue;
        }
        if(channelId>0&&record.channelId()!=channelId){
            continue;
        }
        if (hour >= 0 && record.createTime().time().hour() != hour) continue;
        if (eventType >= 0 && record.eventType() != eventType) continue;
        filtered.append(record);
    }

    std::sort(filtered.begin(),filtered.end(),[](const imageinfo &left,
                                                   const imageinfo &right){
        if (left.createTime() != right.createTime()) return left.createTime()>right.createTime();
        return left.imagePath() < right.imagePath();
    });

    total=filtered.size();
    const int first=qBound(0,offset,total);
    const int last=qMin(total,first+qMax(0,limit));
    for(int index=first;index<last;++index){
        images.append(filtered.at(index));
    }
    return true;
}
