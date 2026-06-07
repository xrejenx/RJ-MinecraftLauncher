#include <QString>
#include <QMap>

class FetchDetail {
public:
    static void storeReleaseNotes(const QString &version, const QString &notes) {
        m_notesCache[version] = notes;
    }
    static QString getReleaseNotes(const QString &version) {
        return m_notesCache.value(version, "No details available.");
    }
private:
    static QMap<QString, QString> m_notesCache;
};
