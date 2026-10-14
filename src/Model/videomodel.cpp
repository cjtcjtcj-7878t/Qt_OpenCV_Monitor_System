#include "videomodel.h"

#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>

#include <algorithm>

namespace
{
QString normalizedVideoPath(const QString &path,const QString &videoDirectory)
{
    const QString trimmedPath=path.trimmed();
    if(trimmedPath.isEmpty()){
        return QString();
    }

    // 数据库中可能保存绝对路径，也可能只保存文件名或相对路径。
    const QString absolutePath=QDir::isAbsolutePath(trimmedPath)?trimmedPath:QDir(videoDirectory).filePath(trimmedPath);
    return QDir::cleanPath(QFileInfo(absolutePath).absoluteFilePath());
}

QString pathKey(const QString &path)
{
    // Windows 路径大小写不敏感，统一大小写后才能可靠匹配数据库记录。
    return QDir::cleanPath(path).toCaseFolded();
}

bool isDetectionVideoName(const QString &fileName)
{
    return fileName.startsWith("detect_camera_",Qt::CaseInsensitive);
}

int cameraIdFromVideoName(const QString &fileName)
{
    static const QRegularExpression pattern(
        "^(?:detect_)?camera_(\\d+)_",
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match=pattern.match(fileName);
    return match.hasMatch()?match.captured(1).toInt():0;
}

bool isSameDate(const QDateTime &time,const QDate &date)
{
    return !date.isValid()||!time.isValid()||time.date()==date;
}
}

videomodel::videomodel() {}

bool videomodel::queryVideolist(int channelid,QDate date,int limit,int offset,const QString &videoDirectory,QList<video> &videoList)
{
    return queryTablePage("video_t",channelid,date,limit,offset,videoDirectory,videoList);
}

bool videomodel::queryDetectVideolist(int channelid,QDate date,int limit,int offset,const QString &videoDirectory,QList<video> &videoList)
{
    return queryTablePage("detect_t",channelid,date,limit,offset,videoDirectory,videoList);
}

bool videomodel::queryTable(const QString &tableName,int channelid,QDate date,const QString &videoDirectory,QList<video> &videoList)
{
    videoList.clear();

    const QString trimmedDirectory=videoDirectory.trimmed();
    if(trimmedDirectory.isEmpty()){
        return true;
    }

    const QDir directory(trimmedDirectory);
    if(!directory.exists()){
        return true;
    }

    // 先读取数据库中已有信息，但不再让数据库决定“文件是否显示”。
    // 即使数据库暂时连接失败，磁盘中的视频仍然可以出现在回放列表。
    QHash<QString,video> recordsByPath;
    QHash<QString,video> recordsByName;
    QHash<int,int> channelByCamera;
    QSqlDatabase db=mySqlconnect::getOne_instance()->getDb();
    const bool databaseAvailable=db.isValid()&&db.open();
    if(databaseAvailable){
        QSqlQuery cameraQuery(db);
        if(cameraQuery.exec("select Id,Channel_id from camera_t")){
            while(cameraQuery.next()){
                channelByCamera.insert(cameraQuery.value("Id").toInt(),cameraQuery.value("Channel_id").toInt());
            }
        }

        // 使用 left join，避免 camera_t 中暂时缺记录时把视频全部过滤掉。
        const QString sql=QString(
            "select t.Id,t.Videoname,t.Video_path,t.Camera_id,t.file_size,"
            "t.start_time,t.end_time,t.create_time,c.Channel_id "
            "from %1 t left join camera_t c on c.Id=t.Camera_id")
            .arg(tableName);
        QSqlQuery query(db);
        query.prepare(sql);
        if(query.exec()){
            while(query.next()){
                video record;
                record.setVideo_id(query.value("Id").toInt());
                record.setVideo_name(query.value("Videoname").toString());
                record.setVideo_path(normalizedVideoPath(
                    query.value("Video_path").toString(),trimmedDirectory));
                record.setCamera_id(query.value("Camera_id").toInt());
                record.setChannel_id(query.value("Channel_id").toInt());
                record.setFile_size(query.value("file_size").toLongLong());
                record.setStart_time(query.value("start_time").toDateTime());
                record.setEnd_time(query.value("end_time").toDateTime());
                record.setCreate_time(query.value("create_time").toDateTime());

                const QString normalizedPath=pathKey(record.getVideo_path());
                if(!normalizedPath.isEmpty()){
                    recordsByPath.insert(normalizedPath,record);
                }
                const QString nameKey=record.getVideo_name().toCaseFolded();
                if(!nameKey.isEmpty()&&!recordsByName.contains(nameKey)){
                    recordsByName.insert(nameKey,record);
                }
            }
        }else{
            qDebug()<<tableName<<"查询数据库记录失败:"<<query.lastError().text();
        }
    }else{
        qDebug()<<"数据库不可用，回放列表仅使用磁盘文件:"<<db.lastError().text();
    }

    const QStringList videoFilters={
        "*.avi","*.AVI","*.mp4","*.MP4","*.mkv","*.MKV",
        "*.mov","*.MOV","*.wmv","*.WMV","*.flv","*.FLV",
        "*.webm","*.WEBM"};
    const QFileInfoList files=directory.entryInfoList(
        videoFilters,
        QDir::Files|QDir::Readable,
        QDir::Time);

    for(const QFileInfo &fileInfo:files){
        const QString filePath=QDir::cleanPath(fileInfo.absoluteFilePath());
        const QString fileKey=pathKey(filePath);
        const bool detectionName=isDetectionVideoName(fileInfo.fileName());
        const bool hasPathRecord=recordsByPath.contains(fileKey);
        const QString nameKey=fileInfo.fileName().toCaseFolded();
        const bool hasNameRecord=recordsByName.contains(nameKey);

        // 当前程序用文件名前缀区分两种录像；旧记录若能在数据库中匹配，仍以数据库所属表为准。
        if(tableName=="detect_t"){
            if(!detectionName&&!hasPathRecord&&!hasNameRecord){
                continue;
            }
        }else if(detectionName&&!hasPathRecord&&!hasNameRecord){
            continue;
        }

        video record;
        if(hasPathRecord){
            record=recordsByPath.value(fileKey);
        }else if(hasNameRecord){
            record=recordsByName.value(nameKey);
        }else{
            // 没有数据库记录时也先显示文件，后续仍可通过双击直接播放。
            const int cameraId=cameraIdFromVideoName(fileInfo.fileName());
            const QDateTime fileTime=fileInfo.lastModified();
            record.setVideo_name(fileInfo.fileName());
            record.setVideo_path(filePath);
            record.setCamera_id(cameraId);
            record.setChannel_id(channelByCamera.value(cameraId,0));
            record.setFile_size(fileInfo.size());
            record.setStart_time(fileTime);
            record.setEnd_time(fileTime);
            record.setCreate_time(fileTime);
        }

        // 文件路径以当前目录中的真实路径为准，避免数据库保存旧目录时播放失败。
        record.setVideo_path(filePath);
        if(record.getChannel_id()<=0){
            // 数据库记录缺少通道信息时，用 camera_t 的最新配置补齐。
            record.setChannel_id(channelByCamera.value(record.getCamera_id(),0));
        }
        if(record.getVideo_name().trimmed().isEmpty()){
            record.setVideo_name(fileInfo.fileName());
        }
        if(record.getFile_size()<=0){
            record.setFile_size(fileInfo.size());
        }
        if(!record.getCreate_time().isValid()){
            // 没有数据库时间时，使用文件最后修改时间参与日期筛选和排序。
            record.setCreate_time(fileInfo.lastModified());
        }
        if(record.getCreate_time().isValid()
           &&!isSameDate(record.getCreate_time(),date)){
            continue;
        }
        if(channelid>0&&record.getChannel_id()!=channelid){
            continue;
        }
        videoList.append(record);
    }

    // 无论数据库记录是否完整，列表都按文件对应的创建时间从新到旧显示。
    std::sort(videoList.begin(),videoList.end(),[](const video &left,const video &right){
        const QDateTime leftTime=left.getCreate_time();
        const QDateTime rightTime=right.getCreate_time();
        return leftTime>rightTime;
    });
    return true;
}

bool videomodel::queryTablePage(const QString &tableName,int channelid,QDate date,int limit,int offset,const QString &videoDirectory,QList<video> &videoList)
{
    QList<video> existingFiles;
    if(!queryTable(tableName,channelid,date,videoDirectory,existingFiles)){
        return false;
    }

    // 先完成目录扫描和筛选，再分页，避免目录变化后出现空白页。
    const int first=qBound(0,offset,existingFiles.size());
    const int last=qMin(existingFiles.size(),first+qMax(0,limit));
    for(int index=first;index<last;++index){
        videoList.append(existingFiles.at(index));
    }
    return true;
}

bool videomodel::insertVideo(video &v,QString *errorMessage)
{
    if(errorMessage){
        errorMessage->clear();
    }
    //获取数据库连接
    QSqlDatabase db=mySqlconnect::getOne_instance()->getDb();
    if(!db.open()){
        if(errorMessage){
            *errorMessage=db.lastError().text();
        }
        qDebug()<<"数据库monitor_app账号连接失败";
        return false;
    }

    //sql语句
    QString sql="insert into video_t(Camera_id,file_size,Videoname,Video_path,"
                "start_time,end_time,create_time) "
                "values(:camera_id,:file_size,:name,:path,:start_time,"
                ":end_time,:create_time)";
    QSqlQuery query(db);   //对数据库db执行sql语句

    query.prepare(sql); //执行sql语句

    //不定值赋值
    query.bindValue(":name",v.getVideo_name());
    query.bindValue(":path",v.getVideo_path());
    query.bindValue(":camera_id",v.getCamera_id());
    query.bindValue(":file_size",v.getFile_size());
    query.bindValue(":start_time",v.getStart_time());
    query.bindValue(":end_time",v.getEnd_time());
    query.bindValue(":create_time",v.getCreate_time());

    if(query.exec()){
        if(query.numRowsAffected()>0){
            v.setVideo_id(query.lastInsertId().toInt());
            qDebug()<<"数据库video_t表插入视频成功!";
            return true;
        }
    }
    if(errorMessage){
        *errorMessage=query.lastError().text();
    }
    qDebug()<<query.lastError().text();     //插入失败显示原因
    return false;
}

bool videomodel::finishVideo(int videoId,qint64 fileSize,const QDateTime &endTime,QString *errorMessage)
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
    query.prepare("update video_t set file_size=:file_size,end_time=:end_time "
                  "where Id=:video_id");
    query.bindValue(":file_size",fileSize);
    query.bindValue(":end_time",endTime);
    query.bindValue(":video_id",videoId);

    if(query.exec()&&query.numRowsAffected()>0){
        return true;
    }

    if(errorMessage){
        *errorMessage=query.lastError().text();
        if(errorMessage->isEmpty()){
            *errorMessage="未找到需要更新的视频记录";
        }
    }
    return false;
}

bool videomodel::insertDetectVideo(video &v,QString *errorMessage)
{
    if(errorMessage){
        errorMessage->clear();
    }

    QSqlDatabase db=mySqlconnect::getOne_instance()->getDb();
    if(!db.isValid()||!db.open()){
        if(errorMessage){
            *errorMessage=db.lastError().text();
        }
        return false;
    }

    // detect_t 没有 event_type 字段，只插入表结构中真实存在的列。
    QSqlQuery query(db);
    query.prepare("insert into detect_t(Camera_id,file_size,Videoname,Video_path,"
                  "start_time,end_time,create_time) "
                  "values(:camera_id,:file_size,:name,:path,:start_time,"
                  ":end_time,:create_time)");
    query.bindValue(":camera_id",v.getCamera_id());
    query.bindValue(":file_size",v.getFile_size());
    query.bindValue(":name",v.getVideo_name());
    query.bindValue(":path",v.getVideo_path());
    query.bindValue(":start_time",v.getStart_time());
    query.bindValue(":end_time",v.getEnd_time());
    query.bindValue(":create_time",v.getCreate_time());
    if(query.exec()&&query.numRowsAffected()>0){
        v.setVideo_id(query.lastInsertId().toInt());
        return true;
    }
    if(errorMessage){
        *errorMessage=query.lastError().text();
    }
    return false;
}

bool videomodel::finishDetectVideo(int videoId,qint64 fileSize,const QDateTime &endTime,QString *errorMessage)
{
    if(errorMessage){
        errorMessage->clear();
    }
    QSqlDatabase db=mySqlconnect::getOne_instance()->getDb();
    if(!db.isValid()||!db.open()){
        if(errorMessage){
            *errorMessage=db.lastError().text();
        }
        return false;
    }
    QSqlQuery query(db);
    query.prepare("update detect_t set file_size=:file_size,end_time=:end_time "
                  "where Id=:video_id");
    query.bindValue(":file_size",fileSize);
    query.bindValue(":end_time",endTime);
    query.bindValue(":video_id",videoId);
    if(query.exec()&&query.numRowsAffected()>0){
        return true;
    }
    if(errorMessage){
        *errorMessage=query.lastError().text();
        if(errorMessage->isEmpty()){
            *errorMessage="未找到需要更新的移动侦测视频记录";
        }
    }
    return false;
}

int videomodel::GetVideostotal(int channelid,QDate date,const QString &videoDirectory)
{
    QList<video> existingFiles;
    return queryTable("video_t",channelid,date,videoDirectory,existingFiles)?existingFiles.size():0;
}

int videomodel::GetDetectVideostotal(int channelid,QDate date,const QString &videoDirectory)
{
    QList<video> existingFiles;
    return queryTable("detect_t",channelid,date,videoDirectory,existingFiles)?existingFiles.size():0;
}
