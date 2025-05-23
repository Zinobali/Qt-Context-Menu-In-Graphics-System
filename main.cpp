﻿#include <QApplication>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>
#include <memory>
#include <QGraphicsSceneEvent>
#include <QMap>
#include <QString>
#include <functional>
#include <QDebug>
#include <QList>
#include <QMessageBox>
#include <QClipboard>

//*******************************************************************************************/
//万能类型上下文
//*******************************************************************************************/
class CommandContext {
public:
    void* target = nullptr;                         // 可是任何图元、界面对象
    QVariantMap extras;                             // 存储任意键值扩展
    QGraphicsScene* scene = nullptr;                // 可选：传场景
    QWidget* view = nullptr;                        // 可选：传视图
    QGraphicsItem* item = nullptr;                  // 可选：传图元
};

using CmdCtxPtr = std::shared_ptr<CommandContext>;

//*******************************************************************************************/
//图元
//*******************************************************************************************/
class BaseCustomItem : public QGraphicsItem {
public:
    // 构造函数，使用默认构造函数
    BaseCustomItem() = default;
    virtual ~BaseCustomItem() = default;

    // 类型标识，用于菜单策略工厂匹配
    virtual QString objectType() const = 0;

    virtual void copy() {
        QMessageBox::information(nullptr, "Copy", "Copy action: objectType = " + objectType());
    }
};

class CustomItem : public BaseCustomItem {
public:
    QRectF boundingRect() const override {
        return QRectF(0, 0, 100, 50);
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override {
        painter->setPen(QColor(70, 130, 180));
        painter->drawRect(boundingRect());
        painter->drawText(QPoint(10, 30), "TextItem");
    }

    QString objectType() const override {
        return "TextItem";  // 用于工厂查找
    }
};

class CustomItem2 : public BaseCustomItem {
public:
    QRectF boundingRect() const override {
        return QRectF(0, 0, 100, 50);
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override {
        painter->setPen(QColor(70, 130, 180));
        painter->setBrush(QColor(70, 130, 180));
        painter->drawRect(boundingRect());
    }

    QString objectType() const override {
        return "Special";  // 这里填对应类型字符串，方便工厂查找
    }
};

// Custom QGraphicsItem2
class CustomItem3 : public CustomItem {
public:
    QRectF boundingRect() const override {
        return QRectF(0, 0, 50, 100);
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override {
        painter->setPen(QColor(211, 37, 167));
        painter->setBrush(QColor(211, 37, 167));
        // smooth
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->drawEllipse(boundingRect());
    }

    QString objectType() const override {
        return "Circle";  // 这里填对应类型字符串，方便工厂查找
    }
};


//*******************************************************************************************/
// 右键命令
//*******************************************************************************************/
// 命令接口
class ICommand {
public:
    virtual ~ICommand() = default;
    virtual void execute(CmdCtxPtr ctx) = 0;

    // 控制对应的Action是否启用
    virtual bool isEnable(CmdCtxPtr ctx) const {
        return true;
    }

    // 控制对应的Action是否显示
    virtual bool isVisible(CmdCtxPtr ctx) const {
        return true;
    }
};

// 组合命令(为了以后扩展)
class CompositeCommand : public ICommand {
public:
    void addCommand(std::shared_ptr<ICommand> cmd) {
        if(cmd) commands.push_back(std::move(cmd));
    }

    void execute(CmdCtxPtr ctx) override {
        for (auto& cmd : commands) {
            if(cmd) cmd->execute(ctx);
        }
    }

private:
    std::vector<std::shared_ptr<ICommand>> commands;
};

// 命令组合器，用于生成组合命令
namespace CommandUtils {
std::shared_ptr<ICommand> combineCommands(std::initializer_list<std::shared_ptr<ICommand>> list) {
    auto combo = std::make_shared<CompositeCommand>();
    for (const auto& cmd : list) {
        combo->addCommand(cmd);
    }
    return combo;
}
}

// 复制命令
class CopyCommand : public ICommand {
public:
    void execute(CmdCtxPtr ctx) override {
        // 选中的对象可能是多个
        auto list = ctx->extras.value("selection").value<QList<BaseCustomItem*>>();
        for (BaseCustomItem* item : list) {
            if (item) item->copy();
        }
    }
};

// 粘贴命令
class PasteCommand : public ICommand {
public:
    void execute(CmdCtxPtr ctx) override {
        QClipboard* clipboard = QApplication::clipboard();
        QMessageBox::information(nullptr, "paste", clipboard->text());
    }

    virtual bool isEnable(CmdCtxPtr ctx) const override {
        QClipboard* clipboard = QApplication::clipboard();
        return !clipboard->text().isEmpty();
        // return false;
    }
};

// 空命令，什么也不做
class NullCommand : public ICommand {
public:
    void execute(CmdCtxPtr ctx) override {
        // do nothing
    }
};

// 自定义命令1
class CustomCommand1 : public ICommand {
public:
    void execute(CmdCtxPtr ctx) override {
        qDebug() << "CustomCommand1 executed";
    }
};

// 自定义命令2
class CustomCommand2 : public ICommand {
public:
    void execute(CmdCtxPtr ctx) override {
        qDebug() << "CustomCommand2 executed";
    }
};


//*******************************************************************************************/
// 右键菜单
//*******************************************************************************************/
// 策略接口
class MenuStrategy {
public:
    virtual ~MenuStrategy() = default;
    virtual QMenu* createMenu(QWidget* parent, CmdCtxPtr ctx) = 0;

protected:
    void addCommandAction(QMenu* menu, const QString& text,
                          std::shared_ptr<ICommand> cmd,
                          CmdCtxPtr ctx) {
        if (!cmd || !cmd->isVisible(ctx)) return;

        auto* action = menu->addAction(text);
        action->setEnabled(cmd->isEnable(ctx));
        QObject::connect(action, &QAction::triggered, [cmd, ctx]() {
            cmd->execute(ctx);
        });
    }

    void addCommandAction(QMenu* menu, const QString& text, CmdCtxPtr ctx) {
        addCommandAction(menu, text, std::make_shared<NullCommand>(), ctx);
    }
};

// 基础菜单装饰器，增加“复制剪切粘贴”等基础操作
class BaseMenuDecorator : public MenuStrategy {
public:
    BaseMenuDecorator(std::shared_ptr<MenuStrategy> wrapped)
        : wrappedStrategy(std::move(wrapped)) {}

    QMenu* createMenu(QWidget* parent, CmdCtxPtr ctx) override {
        QMenu* menu = nullptr;
        if (wrappedStrategy) {
            menu = wrappedStrategy->createMenu(parent, ctx);
        } else {
            menu = new QMenu(parent);
        }
        // 添加基础菜单项
        menu->addSeparator();
        addCommandAction(menu, "复制", std::make_shared<CopyCommand>(), ctx);
        addCommandAction(menu, "剪切", ctx);
        addCommandAction(menu, "粘贴", std::make_shared<PasteCommand>(), ctx);
        return menu;
    }

private:
    std::shared_ptr<MenuStrategy> wrappedStrategy;
};

class PasteOnlyMenuDecorator : public MenuStrategy {
public:
    PasteOnlyMenuDecorator(std::shared_ptr<MenuStrategy> wrapped)
        : wrappedStrategy(std::move(wrapped)) {}

    QMenu* createMenu(QWidget* parent, CmdCtxPtr ctx) override {
        QMenu* menu = nullptr;
        if (wrappedStrategy) {
            menu = wrappedStrategy->createMenu(parent, ctx);
        } else {
            menu = new QMenu(parent);
        }

        menu->addSeparator();
        addCommandAction(menu, "粘贴", ctx);
        return menu;
    }

private:
    std::shared_ptr<MenuStrategy> wrappedStrategy;
};

// 文本菜单策略 (基础菜单 + 特殊菜单)
class TextItemMenuStrategy : public MenuStrategy {
public:
    QMenu* createMenu(QWidget* parent, CmdCtxPtr ctx) override {
        QMenu* menu = new QMenu(parent);
        addCommandAction(menu, "编辑文本", ctx);
        addCommandAction(menu, "改变字体", ctx);
        return menu;
    }
};

// 背景菜单策略 (基础菜单 + 特殊菜单)
class BackgroundMenuStrategy : public MenuStrategy {
public:
    QMenu* createMenu(QWidget* parent, CmdCtxPtr ctx) override {
        QMenu* menu = new QMenu(parent);
        addCommandAction(menu, "添加幻灯片", ctx);
        addCommandAction(menu, "版式布局", ctx);
        return menu;
    }
};

// 不支持基础菜单，只显示自己的菜单
class NoBaseMenuStrategy : public MenuStrategy {
public:
    QMenu* createMenu(QWidget* parent, CmdCtxPtr ctx) override {
        QMenu* menu = new QMenu(parent);

        auto combo = std::make_shared<CompositeCommand>();
        combo->addCommand(std::make_shared<CustomCommand1>());
        combo->addCommand(std::make_shared<CustomCommand2>());

        addCommandAction(menu, "无公共操作，仅特殊操作", combo, ctx);
        return menu;
    }
};

// 椭圆菜单策略 (基础菜单 + 特殊菜单)
class CircleMenuStrategy : public MenuStrategy {
public:
    QMenu* createMenu(QWidget* parent, CmdCtxPtr ctx) override {
        QMenu* menu = new QMenu(parent);
        addCommandAction(menu, "改变颜色", ctx);
        addCommandAction(menu, "改变大小", ctx);

        // 命令组合器使用
        auto combo = CommandUtils::combineCommands({
            std::make_shared<CustomCommand1>(),
            std::make_shared<CustomCommand2>()
        });

        // 二级菜单
        QMenu* subMenu = new QMenu("图形属性", menu);
        addCommandAction(subMenu, "旋转", combo, ctx);
        addCommandAction(subMenu, "缩放", ctx);

        menu->addMenu(subMenu);
        return menu;
    }
};


//*******************************************************************************************/
// 工厂
//*******************************************************************************************/
// 菜单策略工厂注册器
class MenuStrategyFactory {
public:
    using Creator = std::function<std::shared_ptr<MenuStrategy>()>;

    // 单例
    static MenuStrategyFactory& GetInstance() {
        static MenuStrategyFactory factory;
        return factory;
    }

    void registerCreator(const QString& type, Creator creator) {
        creators[type] = creator;
    }

    std::shared_ptr<MenuStrategy> create(const QString& type) {
        if (creators.contains(type)) {
            return creators[type]();
        }
        return nullptr;
    }

private:
    QMap<QString, Creator> creators;
};


//*******************************************************************************************/
//场景
//*******************************************************************************************/
// Custom QGraphicsScene
class CustomScene : public QGraphicsScene {
public:
    CustomScene() = default;

protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override {
        QGraphicsItem* item = itemAt(event->scenePos(), QTransform());


        // auto list = ctx.extras.value("selection").value<QList<BaseCustomItem*>>();
        // for (BaseCustomItem* item : list) {
        //     if (item) item->copy();
        // }

        if(item) {
            BaseCustomItem* baseItem = dynamic_cast<BaseCustomItem*>(item);

            // 转换成功
            if (baseItem) {
                auto strategy = MenuStrategyFactory::GetInstance().create(baseItem->objectType());
                if (strategy) {
                    CmdCtxPtr ctx = std::make_shared<CommandContext>();
                    ctx->scene = this;
                    ctx->extras["selection"] = QVariant::fromValue(QList<BaseCustomItem*>() << baseItem);
                    QMenu* menu = strategy->createMenu(nullptr, ctx);
                    menu->exec(event->screenPos());
                    delete menu;
                    return;  // 菜单弹出后返回，防止继续冒泡
                }
            }
        }

        auto defaultStrategy = MenuStrategyFactory::GetInstance().create("Background");
        if (defaultStrategy) {
            CmdCtxPtr ctx = std::make_shared<CommandContext>();
            QMenu* menu = defaultStrategy->createMenu(nullptr, ctx);
            menu->exec(event->screenPos());
            delete menu;
            return;
        }

        QGraphicsScene::contextMenuEvent(event);
    }

private:
    std::shared_ptr<MenuStrategy> menuStrategy;
};

//*******************************************************************************************/
//注册
//*******************************************************************************************/
// 注册各种策略
void registerMenuStrategies() {
    MenuStrategyFactory::GetInstance().registerCreator("TextItem", []() {
        // 装饰基础菜单
        return std::make_shared<BaseMenuDecorator>(std::make_shared<TextItemMenuStrategy>());
    });
    MenuStrategyFactory::GetInstance().registerCreator("Background", []() {
        return std::make_shared<PasteOnlyMenuDecorator>(std::make_shared<BackgroundMenuStrategy>());
    });
    MenuStrategyFactory::GetInstance().registerCreator("Special", []() {
        // 不装饰基础菜单，只有特殊菜单
        return std::make_shared<NoBaseMenuStrategy>();
    });
    MenuStrategyFactory::GetInstance().registerCreator("Circle", []() {
        return std::make_shared<BaseMenuDecorator>(std::make_shared<CircleMenuStrategy>());
    });
}


Q_DECLARE_METATYPE(QList<BaseCustomItem*>)
int main(int argc, char *argv[]) {
    qRegisterMetaType<QList<BaseCustomItem*>>("QList<BaseCustomItem*>");

    QApplication app(argc, argv);

    registerMenuStrategies();

    // 创建场景
    CustomScene* scene = new CustomScene();
    // 添加带基础菜单的元素
    BaseCustomItem* item1 = new CustomItem();
    item1->setPos(50, 50);
    scene->addItem(item1);

    // 添加不支持基础菜单的元素
    BaseCustomItem* item2 = new CustomItem2();
    item2->setPos(200, 50);
    scene->addItem(item2);

    BaseCustomItem* item3 = new CustomItem3();
    item3->setPos(100, 150);
    scene->addItem(item3);

    QGraphicsView* view = new QGraphicsView(scene);
    view->setSceneRect(0, 0, 400, 300);
    view->show();

    return app.exec();
}
