#ifndef RECORDINGTIMELINE_H
#define RECORDINGTIMELINE_H

#include <algorithm>
#include <cmath>
#include <cstdint>

// 按单调时钟重采样为固定帧率。Frame 需提供 empty()/clone()。
// 普通与侦测录像共用；写入回调成功返回 true。
// 不伪造丢失动作：时间空隙保持上一帧，过密输入不重复写入同一时间槽。
template<class Frame>
class RecordingTimeline
{
public:
    using Time = std::int64_t;

    void reset(double fps, Time limitMs)
    {
        fps_ = std::isfinite(fps) && fps > 0 ? fps : 25.0;
        limit_ = std::max<Time>(1, limitMs);
        written_ = 0;
        last_ = Frame();
    }

    template<class Write>
    bool append(const Frame &frame, Time elapsedMs, Write write)
    {
        if (frame.empty()) return false;
        if (last_.empty()) last_ = frame.clone();
        const Time elapsed = std::max<Time>(0, elapsedMs);
        if (elapsed >= limit_) return padTo(limit_, write);
        const Time slot = static_cast<Time>(std::floor(elapsed * fps_ / 1000.0));
        while (written_ < slot) {
            if (!write(last_)) return false;
            ++written_;
        }
        if (written_ == slot) {
            if (!write(frame)) return false;
            ++written_;
        }
        last_ = frame.clone();
        return true;
    }

    template<class Write>
    bool padTo(Time elapsedMs, Write write)
    {
        if (last_.empty()) return true;
        const Time elapsed = std::max<Time>(0, std::min(elapsedMs, limit_));
        const Time count = std::max<Time>(1, static_cast<Time>(
            std::ceil(elapsed * fps_ / 1000.0 - 1e-9)));
        while (written_ < count) {
            if (!write(last_)) return false;
            ++written_;
        }
        return true;
    }

    Time frameCount() const { return written_; }
    Time durationMs() const {
        return static_cast<Time>(std::llround(written_ * 1000.0 / fps_));
    }

private:
    double fps_ = 25.0;
    Time limit_ = 30000;
    Time written_ = 0;
    Frame last_;
};
#endif
