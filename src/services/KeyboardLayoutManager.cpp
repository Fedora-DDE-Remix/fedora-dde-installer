#include "KeyboardLayoutManager.h"
#include "CommonFunc.h"

#include <QDebug>

KeyboardLayoutManager *KeyboardLayoutManager::instance()
{
    static KeyboardLayoutManager kblm;
    return &kblm;
}

KeyboardLayoutManager::KeyboardLayoutManager()
{

}

void KeyboardLayoutManager::readLayoutVariantList(const QDomNode& root
                                                  , XKbLayoutVariantList& variantList)
{
    const QDomNodeList variantNodes(root.childNodes());
    for (int varIdx = 0; varIdx < variantNodes.length(); varIdx++) {
        const QDomNode varNode(variantNodes.at(varIdx).firstChild());
        if (varNode.isNull()) {
            continue;
        }

        SXkbLayoutVariant layoutVariant;
        const QDomNodeList configs(varNode.childNodes());

        for (int cfgIdx = 0; cfgIdx < configs.length(); cfgIdx++) {
            const QDomNode cfg(configs.at(cfgIdx));

            if (cfg.nodeName() == "name") {
                layoutVariant.name = cfg.toElement().text();
                continue;
            }

            if (cfg.nodeName() == "description") {
                layoutVariant.description = cfg.toElement().text();
                continue;
            }

            if (cfg.nodeName() == "shortDescription") {
                layoutVariant.shortDescription = cfg.toElement().text();
                continue;
            }

            if (cfg.nodeName() == "languageList") {
                const QDomNodeList langNodes(cfg.childNodes());

                for (int langIdx = 0; langIdx < langNodes.length(); langIdx++) {
                    layoutVariant.languageList.append(langNodes.at(langIdx).toElement().text());
                }

                continue;
            }
        }

        variantList.append(layoutVariant);
    }
}

void KeyboardLayoutManager::readLayoutConfigItem(const QDomNode& root, SXkbLayout& xkbLayout)
{
    const QDomNodeList props(root.childNodes());
    for (int propIdx = 0; propIdx < props.length(); propIdx++) {
        const QDomNode prop(props.at(propIdx));

        if (prop.nodeName() == "name") {
            xkbLayout.name = prop.toElement().text();
            continue;
        }

        if (prop.nodeName() == "description") {
            xkbLayout.description = prop.toElement().text();
            continue;
        }

        if (prop.nodeName() == "shortDescription") {
            xkbLayout.shortDescription = prop.toElement().text();
            continue;
        }

        if (prop.nodeName() == "languageList") {
            const QDomNodeList langNodes(prop.childNodes());

            for (int langIdx = 0; langIdx < langNodes.length(); langIdx++) {
                xkbLayout.languageList.append(langNodes.at(langIdx).toElement().text());
            }

            continue;
        }
    }
}

void KeyboardLayoutManager::readLayoutList(const QDomNode& root, XkbLayoutList& layoutList)
{
    const QDomNodeList layoutNodes(root.childNodes());
    for (int layoutIdx = 0; layoutIdx < layoutNodes.length(); layoutIdx++) {
        SXkbLayout xkbLayout;
        const QDomNodeList contentNodes(layoutNodes.at(layoutIdx).childNodes());

        for (int contentIdx = 0; contentIdx < contentNodes.length(); contentIdx++) {
            const QDomNode contentNode(contentNodes.at(contentIdx));

            if (contentNode.nodeName() == "configItem") {
                readLayoutConfigItem(contentNode, xkbLayout);
            }

            if (contentNode.nodeName() == "variantList") {
                readLayoutVariantList(contentNode, xkbLayout.variantList);
            }
        }

        layoutList.append(xkbLayout);
    }
}

bool KeyboardLayoutManager::readConfig(const QString& filepath, XkbLayoutList& layoutList)
{
    QDomDocument doc;
    QDomDocument::ParseResult result = doc.setContent(CommonFunc::readFile(filepath));
    if (!result.errorMessage.isEmpty()) {
        return false;
    }
    
    const QDomNodeList nodes(doc.documentElement().childNodes());

    for (int index = 0; index < nodes.length(); index++) {
        const QDomNode node = nodes.at(index);

        if (node.nodeName() == "layoutList") {
            readLayoutList(node, layoutList);
        }
    }

    return true;
}

void KeyboardLayoutManager::xkbLayoutMap(LayoutVariantsMap& layoutVariants)
{
    const char xkbBaseRule[] = "/usr/share/X11/xkb/rules/base.xml";
    XkbLayoutList layoutList;

    if (!readConfig(xkbBaseRule, layoutList)) {
        qWarning() << "Failed to read xkb config file" << xkbBaseRule;
        return;
    }

    XKbLayoutVariantList variantList;
    // layout description为Chinese的项
    QStringList insertLayoutList;

    for (SXkbLayout& layout : layoutList) {
        // 把layout信息项挂接到variantList的首位置
        SXkbLayoutVariant variant;
        variant.name = layout.name;
        variant.layoutName = layout.name;
        variant.description = layout.description;
        variant.shortDescription = layout.shortDescription;
        variant.languageList = layout.languageList;
        layout.variantList.prepend(variant);

        // 设置variantList的layoutName
        for (SXkbLayoutVariant& var : layout.variantList) {
            var.layoutName = layout.name;
        }

        // 把Taiwanese的variantList提取出来，暂不合入
        if (layout.description == "Taiwanese" || layout.description == "Chinese Taiwan") {
            variantList << layout.variantList;
        } else {
            layoutVariants[layout.name] = layout.variantList;

            if (layout.description == "Chinese") {
                insertLayoutList << layout.name;
            }
        }
    }

    if (insertLayoutList.count() < 1) {
        qWarning() << "No layout description is Chinese, so drop Taiwanese's variantList";
    } else if (insertLayoutList.count() > 1) {
        qWarning() << "Multi layout description is Chinese";
    } else {
        // 将上面提取的variantList合并到Chinese中
        if (layoutVariants.contains(insertLayoutList.first())) {
            layoutVariants[insertLayoutList.first()].append(variantList);
        }
    }
}
