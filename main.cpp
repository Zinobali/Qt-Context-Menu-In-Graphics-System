#include <QApplication>
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
// 选中上下文    (全局单例)
//*******************************************************************************************/
class SelectionContext {
public:
    static SelectionContext& GetInstance() {
        static SelectionContext ctx;
        return ctx;
    }

    void setSelectedItem(BaseCustomItem* item) {
        // 单选
        selectedItems.clear();
        if (item) selectedItems.append(item);
    }

    void setSelectedItems(const QList<BaseCustomItem*>& items) {
        // 拖动框选多个
        selectedItems.clear();
        selectedItems = items;
    }

    const QList<BaseCustomItem*>& getSelectedItems() const {
        return selectedItems;
    }

    void clear() { selectedItems.clear(); }

private:
    QList<BaseCustomItem*> selectedItems;    
};


//*******************************************************************************************/
// 右键命令
//*******************************************************************************************/
// 命令接口
class ICommand {
public:
    virtual ~ICommand() = default;
    virtual void execute() = 0;
};

class CopyCommand : public ICommand {
public:
    void execute() override {
        const auto& items = SelectionContext::GetInstance().getSelectedItems();
        for (auto* item : items) {
            if (item) item->copy();
        }
    }
};

class NullCommand : public ICommand {
public:
    void execute() override {
        // do nothing
    }
};


//*******************************************************************************************/
// 右键菜单
//*******************************************************************************************/
// 策略接口
class MenuStrategy {
public:
    virtual ~MenuStrategy() = default;
    virtual QMenu* createMenu(QWidget* parent) = 0;

protected:
    void addCommandAction(QMenu* menu, const QString& text, std::shared_ptr<ICommand> cmd) {
        auto* action = menu->addAction(text);
        QObject::connect(action, &QAction::triggered, [cmd]() { 
            cmd->execute();
        });
    }

    void addCommandAction(QMenu* menu, const QString& text) {
        addCommandAction(menu, text, std::make_shared<NullCommand>());
    }
};

// 基础菜单装饰器，增加“复制剪切粘贴”等基础操作
class BaseMenuDecorator : public MenuStrategy {
public:
    BaseMenuDecorator(std::shared_ptr<MenuStrategy> wrapped)
        : wrappedStrategy(std::move(wrapped)) {}

    QMenu* createMenu(QWidget* parent) override {
        QMenu* menu = nullptr;
        if (wrappedStrategy) {
            menu = wrappedStrategy->createMenu(parent);
        } else {
            menu = new QMenu(parent);
        }
        // 添加基础菜单项
        menu->addSeparator();
        addCommandAction(menu, QString::fromLocal8Bit("复制"), std::make_shared<CopyCommand>());
        addCommandAction(menu, QString::fromLocal8Bit("剪切"));
        addCommandAction(menu, QString::fromLocal8Bit("粘贴"));
        return menu;
    }

private:
    std::shared_ptr<MenuStrategy> wrappedStrategy;
};

class PasteOnlyMenuDecorator : public MenuStrategy {
public:
    PasteOnlyMenuDecorator(std::shared_ptr<MenuStrategy> wrapped)
        : wrappedStrategy(std::move(wrapped)) {}

    QMenu* createMenu(QWidget* parent) override {
        QMenu* menu = nullptr;
        if (wrappedStrategy) {
            menu = wrappedStrategy->createMenu(parent);
        } else {
            menu = new QMenu(parent);
        }

        menu->addSeparator();
        addCommandAction(menu, QString::fromLocal8Bit("粘贴"));
        return menu;
    }

private:
    std::shared_ptr<MenuStrategy> wrappedStrategy;
};

// 文本菜单策略 (基础菜单 + 特殊菜单)
class TextItemMenuStrategy : public MenuStrategy {
public:
    QMenu* createMenu(QWidget* parent) override {
        QMenu* menu = new QMenu(parent);
        addCommandAction(menu, QString::fromLocal8Bit("编辑文本"));
        addCommandAction(menu, QString::fromLocal8Bit("改变字体"));
        return menu;
    }
};

// 背景菜单策略 (基础菜单 + 特殊菜单)
class BackgroundMenuStrategy : public MenuStrategy {
public:
    QMenu* createMenu(QWidget* parent) override {
        QMenu* menu = new QMenu(parent);
        addCommandAction(menu, QString::fromLocal8Bit("添加幻灯片"));
        addCommandAction(menu, QString::fromLocal8Bit("版式布局"));
        return menu;
    }
};

// 不支持基础菜单，只显示自己的菜单
class NoBaseMenuStrategy : public MenuStrategy {
public:
    QMenu* createMenu(QWidget* parent) override {
        QMenu* menu = new QMenu(parent);
        addCommandAction(menu, QString::fromLocal8Bit("无公共操作，仅特殊操作"));
        return menu;
    }
};

// 椭圆菜单策略 (基础菜单 + 特殊菜单)
class CircleMenuStrategy : public MenuStrategy {
public:
    QMenu* createMenu(QWidget* parent) override {
        QMenu* menu = new QMenu(parent);
        addCommandAction(menu, QString::fromLocal8Bit("改变颜色"));
        addCommandAction(menu, QString::fromLocal8Bit("改变大小"));
        
        // 二级菜单
        QMenu* subMenu = new QMenu(QString::fromLocal8Bit("图形属性"), menu);
        addCommandAction(subMenu, QString::fromLocal8Bit("旋转"));
        addCommandAction(subMenu, QString::fromLocal8Bit("缩放"));
        
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

        if(item) {
            BaseCustomItem* baseItem = dynamic_cast<BaseCustomItem*>(item);
            
            // 转换成功
            if (baseItem) {
                SelectionContext::GetInstance().setSelectedItem(baseItem);
                auto strategy = MenuStrategyFactory::GetInstance().create(baseItem->objectType());
                if (strategy) {
                    QMenu* menu = strategy->createMenu(nullptr);
                    menu->exec(event->screenPos());
                    delete menu;
                    return;  // 菜单弹出后返回，防止继续冒泡
                }
            }
        }

        auto defaultStrategy = MenuStrategyFactory::GetInstance().create("Background");
        if (defaultStrategy) {
            QMenu* menu = defaultStrategy->createMenu(nullptr);
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



int main(int argc, char *argv[]) {
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
