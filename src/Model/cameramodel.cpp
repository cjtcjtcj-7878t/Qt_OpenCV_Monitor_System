#include "cameramodel.h"

#include "camerainfo.h"
#include "../util/mysqlconnect.h"

#include <QSqlError>
#include <QSqlQuery>

bool cameramodel::findByChannel(int channelId,int &cameraId,int &cameraIndex,QString *errorMessage) const
{
    cameraId=0;
    cameraIndex=-1;
    if(errorMessage){
        errorMessage->clear();
    }

    if(channelId<=0){
        if(errorMessage){
            *errorMessage="摄像头通道号无效";
        }
        return false;
    }

    QSqlDatabase db=mySqlconnect::getOne_instance()->getDb();
    if(!db.isValid()||!db.open()){
        if(errorMessage){
            *errorMessage=db.lastError().text();
        }
        return false;
    }

    QSqlQuery query(db);
    query.prepare("select Id,Camera_index from camera_t "
                  "where Channel_id=:channel_id limit 1");
    query.bindValue(":channel_id",channelId);
    if(!query.exec()){
        if(errorMessage){
            *errorMessage=query.lastError().text();
        }
        return false;
    }

    if(!query.next()){
        if(errorMessage){
            *errorMessage=QString("camera_t中没有通道号为%1的摄像头记录").arg(channelId);
        }
        return false;
    }

    cameraId=query.value("Id").toInt();
    cameraIndex=query.value("Camera_index").toInt();
    if(cameraId<=0||cameraIndex<0){
        if(errorMessage){
            *errorMessage="camera_t中的摄像头主键或OpenCV设备编号无效";
        }
        return false;
    }
    return true;
}

bool cameramodel::syncCameras(const QList<camerainfo> &cameras,
                              QString *errorMessage) const
{
    if(errorMessage){
        errorMessage->clear();
    }

    if(cameras.size()!=4){
        if(errorMessage){
            *errorMessage="摄像头配置数量必须为4个";
        }
        return false;
    }

    QSqlDatabase db=mySqlconnect::getOne_instance()->getDb();
    if(!db.isValid()||!db.open()){
        if(errorMessage){
            *errorMessage=db.lastError().text();
        }
        return false;
    }
    if(!db.transaction()){
        if(errorMessage){
            *errorMessage=db.lastError().text();
        }
        return false;
    }

    auto fail=[&](const QString &message){
        db.rollback();
        if(errorMessage){
            *errorMessage=message;
        }
        return false;
    };

    for(const camerainfo &camera : cameras){
        const int channelId=camera.getChannelId();
        if(channelId<=0||camera.getChannelName().trimmed().isEmpty()){
            return fail("摄像头通道号或通道名称无效");
        }

        int cameraId=0;
        QSqlQuery selectQuery(db);
        selectQuery.prepare("select Id from camera_t where Channel_id=:channel_id limit 1");
        selectQuery.bindValue(":channel_id",channelId);
        if(!selectQuery.exec()){
            return fail(selectQuery.lastError().text());
        }
        if(selectQuery.next()){
            cameraId=selectQuery.value(0).toInt();
        }

        if(cameraId>0){
            QSqlQuery updateQuery(db);
            updateQuery.prepare("update camera_t set Camera_index=:camera_index,"
                                "Cameraname=:camera_name,status_camera=:status_camera "
                                "where Id=:camera_id");
            updateQuery.bindValue(":camera_index",camera.getCameraIndex());
            updateQuery.bindValue(":camera_name",camera.getChannelName().trimmed());
            updateQuery.bindValue(":status_camera",camera.getCameraIndex()>=0 ? 1 : 0);
            updateQuery.bindValue(":camera_id",cameraId);
            if(!updateQuery.exec()){
                return fail(updateQuery.lastError().text());
            }
        }else{
            QSqlQuery insertQuery(db);
            insertQuery.prepare("insert into camera_t(Channel_id,Camera_index,"
                                "Cameraname,status_camera) values(:channel_id,"
                                ":camera_index,:camera_name,:status_camera)");
            insertQuery.bindValue(":channel_id",channelId);
            insertQuery.bindValue(":camera_index",camera.getCameraIndex());
            insertQuery.bindValue(":camera_name",camera.getChannelName().trimmed());
            insertQuery.bindValue(":status_camera",camera.getCameraIndex()>=0 ? 1 : 0);
            if(!insertQuery.exec()){
                return fail(insertQuery.lastError().text());
            }
        }
    }

    if(!db.commit()){
        if(errorMessage){
            *errorMessage=db.lastError().text();
        }
        db.rollback();
        return false;
    }
    return true;
}
