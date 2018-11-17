#include "QDebugHelper.h"

QDebug operator<<(QDebug qd, const std::thread::id& threadId)
{
    std::stringstream ss;
    ss << threadId;
    qd << ss.str().c_str();
    return qd;
}

QDebug operator<<(QDebug qd, const std::string& s)
{
    qd << s.c_str();
    return qd;
}
QDebug operator<<(QDebug qd, const fs::path& p)
{
    qd << p.string();
    return qd;
}

QString toQString(const QEvent * ev)
{
    if (ev)
    {
        static int eventEnumIndex = QEvent::staticMetaObject.indexOfEnumerator("Type");
        const auto type = ev->type();
        return QEvent::staticMetaObject.enumerator(eventEnumIndex).valueToKey(type);
    }
    else
    {
        return "null";
    }

}

QDebug operator<<(QDebug str, const QEvent * ev) {
    str << "QEvent";
    if (ev)
    {
        const auto name = toQString(ev);
        if (!name.isEmpty()) str << name; else str << ev->type();
    }
    else
    {
      str << static_cast<const void*>(ev);
    }
    return str.maybeSpace();
}

