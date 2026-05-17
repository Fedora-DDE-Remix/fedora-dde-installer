#ifndef KEYBOARDLAYOUTMANAGER_H
#define KEYBOARDLAYOUTMANAGER_H

#include <QStringList>
#include <QDomDocument>

// 下面的XkbLayoutList，XKbLayoutVariantList是忠于原xml文件的结构。
struct SXkbLayoutVariant {
    QString name;
    QString layoutName;
    QString description;
    QString shortDescription;
    QStringList languageList;
};
typedef QList<SXkbLayoutVariant> XKbLayoutVariantList;

struct SXkbLayout {
    QString name;
    QString description;
    QString shortDescription;
    QStringList languageList;
    XKbLayoutVariantList variantList;
};
typedef QList<SXkbLayout> XkbLayoutList;

// 键盘处理的业务采用LayoutVariantsMap数据结构
typedef QMap<QString, XKbLayoutVariantList> LayoutVariantsMap;

class KeyboardLayoutManager
{
public:
    static KeyboardLayoutManager* instance();

    // 读取系统键盘布局的本地化描述。
    void xkbLayoutMap(LayoutVariantsMap& layoutVariants);

private:
    KeyboardLayoutManager();
    KeyboardLayoutManager(const KeyboardLayoutManager&) = delete;

    void readLayoutVariantList(const QDomNode& root,
                               XKbLayoutVariantList& variantList);

    void readLayoutConfigItem(const QDomNode& root, SXkbLayout& xkbLayout);

    void readLayoutList(const QDomNode& root, XkbLayoutList& layoutList);

    bool readConfig(const QString& filepath, XkbLayoutList& layoutList);
};

#endif // KEYBOARDLAYOUTMANAGER_H
