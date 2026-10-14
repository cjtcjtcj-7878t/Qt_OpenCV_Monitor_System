#include "imagecontrol.h"

bool imagecontrol::queryImagePage(int channelId,
                                  const QDate &date,
                                  int limit,
                                  int offset,
                                  const QString &imageDirectory,
                                  const QString &videoDirectory,
                                  QList<imageinfo> &images,
                                  int &total,
                                  QString &errorMessage, int hour, int eventType) const
{
    return model.queryImages(channelId,
                             date,
                             limit,
                             offset,
                             imageDirectory,
                             videoDirectory,
                             images,
                             total,
                             &errorMessage, hour, eventType);
}
