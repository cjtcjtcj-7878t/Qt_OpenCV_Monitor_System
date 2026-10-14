#ifndef IMAGEHOVERPREVIEW_H
#define IMAGEHOVERPREVIEW_H

#include <QAbstractItemView>
#include <QApplication>
#include <QCursor>
#include <QEvent>
#include <QFileInfo>
#include <QGuiApplication>
#include <QIcon>
#include <QImageReader>
#include <QLabel>
#include <QMouseEvent>
#include <QPersistentModelIndex>
#include <QPixmap>
#include <QScreen>
#include <QScrollBar>
#include <QTimer>
#include <algorithm>

// 每条记录第 0 列的 PathRole 必须保存已经解析好的图片绝对路径。
// 支持 QTableWidget、QTableView、QListWidget，无需 Q_OBJECT 或新增 cpp。
class ImageHoverPreview final : public QObject
{
public:
    enum { PathRole = Qt::UserRole + 110 };

    explicit ImageHoverPreview(QAbstractItemView *view)
        : QObject(view), view_(view), popup_(new QLabel(view, Qt::ToolTip))
    {
        popup_->setAttribute(Qt::WA_ShowWithoutActivating);
        popup_->setAttribute(Qt::WA_TransparentForMouseEvents);
        popup_->setAlignment(Qt::AlignCenter);
        popup_->setMargin(8);
        popup_->setStyleSheet("QLabel { background:#172033; color:white; "
                              "border:1px solid #64748b; border-radius:4px; }");
        popup_->hide();
        timer_.setSingleShot(true);
        timer_.setInterval(250);
        view_->setMouseTracking(true);
        view_->viewport()->setMouseTracking(true);
        view_->installEventFilter(this);
        view_->viewport()->installEventFilter(this);
        connect(&timer_, &QTimer::timeout, this, [this]() { showPreview(); });
        connect(view_->verticalScrollBar(), &QScrollBar::valueChanged,
                this, [this](int) { dismiss(); });
        connect(view_->horizontalScrollBar(), &QScrollBar::valueChanged,
                this, [this](int) { dismiss(); });
        // 在列表已经 setModel() 后创建本组件；更换整个 model 时重新创建。
        if (view_->model()) {
            connect(view_->model(), &QAbstractItemModel::modelAboutToBeReset,
                    this, [this]() { dismiss(); });
            connect(view_->model(), &QAbstractItemModel::rowsAboutToBeRemoved,
                    this, [this]() { dismiss(); });
            connect(view_->model(), &QAbstractItemModel::layoutAboutToBeChanged,
                    this, [this]() { dismiss(); });
            connect(view_->model(), &QAbstractItemModel::dataChanged,
                    this, [this]() { dismiss(); });
        }
    }

    // 填充当前页记录时生成缩略图，避免一次解码全部历史图片。
    static QIcon thumbnail(const QString &absolutePath)
    {
        if (!QFileInfo(absolutePath).isAbsolute()) return {};
        QImageReader reader(absolutePath);
        reader.setAutoTransform(true);
        QSize size = reader.size();
        if (size.isValid())
            reader.setScaledSize(size.scaled(128, 88, Qt::KeepAspectRatio));
        const QImage image = reader.read();
        return image.isNull() ? QIcon() : QIcon(QPixmap::fromImage(image));
    }

protected:
    bool eventFilter(QObject *object, QEvent *event) override
    {
        if (object == view_->viewport() && event->type() == QEvent::MouseMove) {
            const auto *mouse = static_cast<QMouseEvent *>(event);
            QModelIndex index = view_->indexAt(mouse->position().toPoint());
            if (index.isValid()) index = index.siblingAtColumn(0);
            if (!index.isValid()) {
                dismiss();
            } else if (index != hovered_) {
                dismiss();
                hovered_ = index;
                timer_.start();
            } else if (popup_->isVisible()) {
                placePopup();
            }
        } else if (event->type() == QEvent::Leave
                   || event->type() == QEvent::Hide
                   || event->type() == QEvent::Wheel
                   || event->type() == QEvent::MouseButtonPress
                   || event->type() == QEvent::WindowDeactivate) {
            dismiss();
        }
        return QObject::eventFilter(object, event);
    }

private:
    void dismiss()
    {
        timer_.stop();
        popup_->hide();
        hovered_ = QPersistentModelIndex();
    }

    void showPreview()
    {
        if (!hovered_.isValid() || !view_->isVisible()) return;
        const QPoint local = view_->viewport()->mapFromGlobal(QCursor::pos());
        QModelIndex current = view_->indexAt(local);
        if (!current.isValid() || current.siblingAtColumn(0) != hovered_) {
            dismiss();
            return;
        }
        const QString path = hovered_.data(PathRole).toString();
        popup_->clear();
        if (path.isEmpty() || !QFileInfo(path).isAbsolute()) {
            popup_->setText("该记录未配置有效的图片绝对路径");
        } else {
            QScreen *screen = QGuiApplication::screenAt(QCursor::pos());
            if (!screen) screen = QGuiApplication::primaryScreen();
            if (!screen) return;
            const QSize available = screen->availableGeometry().size();
            const QSize limit(std::max(1, std::min(760, available.width() - 40)),
                              std::max(1, std::min(560, available.height() - 40)));
            QImageReader reader(path);
            reader.setAutoTransform(true);
            const QSize source = reader.size();
            if (source.isValid())
                reader.setScaledSize(source.scaled(limit, Qt::KeepAspectRatio));
            const QImage image = reader.read();
            if (image.isNull()) {
                popup_->setText("图片不存在或无法读取");
            } else {
                popup_->setPixmap(QPixmap::fromImage(image).scaled(
                    limit, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
        }
        popup_->adjustSize();
        placePopup();
        popup_->show();
    }

    void placePopup()
    {
        const QPoint cursor = QCursor::pos();
        QScreen *screen = QGuiApplication::screenAt(cursor);
        if (!screen) screen = QGuiApplication::primaryScreen();
        if (!screen) return;
        const QRect bounds = screen->availableGeometry();
        int x = cursor.x() + 18;
        int y = cursor.y() + 18;
        if (x + popup_->width() > bounds.right() + 1)
            x = cursor.x() - popup_->width() - 18;
        if (y + popup_->height() > bounds.bottom() + 1)
            y = cursor.y() - popup_->height() - 18;
        x = std::max(bounds.left(), std::min(x, bounds.right() - popup_->width() + 1));
        y = std::max(bounds.top(), std::min(y, bounds.bottom() - popup_->height() + 1));
        popup_->move(x, y);
    }

    QAbstractItemView *view_;
    QLabel *popup_;
    QTimer timer_;
    QPersistentModelIndex hovered_;
};
#endif // IMAGEHOVERPREVIEW_H
