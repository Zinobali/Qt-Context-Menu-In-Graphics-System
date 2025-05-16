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
#include <iostream>

//*******************************************************************************************/
//右键菜单
//*******************************************************************************************/
// Strategy interface
class MenuStrategy {
public:
    virtual ~MenuStrategy() = default;
    virtual QMenu* createMenu(QWidget* parent) = 0;
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
        menu->addAction(QString::fromLocal8Bit("复制"));
        menu->addAction(QString::fromLocal8Bit("剪切"));
        menu->addAction(QString::fromLocal8Bit("粘贴"));
        return menu;
    }

private:
    std::shared_ptr<MenuStrategy> wrappedStrategy;
};

// Concrete strategy for text item (基础菜单 + 特殊菜单)
class TextItemMenuStrategy : public MenuStrategy {
public:
    QMenu* createMenu(QWidget* parent) override {
        QMenu* menu = new QMenu(parent);
        menu->addAction(QString::fromLocal8Bit("编辑文本"));
        menu->addAction(QString::fromLocal8Bit("改变字体"));
        return menu;
    }
};

// Concrete strategy for background (基础菜单 + 特殊菜单)
class BackgroundMenuStrategy : public MenuStrategy {
public:
    QMenu* createMenu(QWidget* parent) override {
        QMenu* menu = new QMenu(parent);
        menu->addAction(QString::fromLocal8Bit("添加幻灯片"));
        menu->addAction(QString::fromLocal8Bit("版式布局"));
        return menu;
    }
};

// 不支持基础菜单，只显示自己的菜单
class NoBaseMenuStrategy : public MenuStrategy {
public:
    QMenu* createMenu(QWidget* parent) override {
        QMenu* menu = new QMenu(parent);
        menu->addAction(QString::fromLocal8Bit("无公共操作，仅特殊操作"));
        return menu;
    }
};

// Concrete strategy for Circle (基础菜单 + 特殊菜单)
class CircleMenuStrategy : public MenuStrategy {
public:
    QMenu* createMenu(QWidget* parent) override {
        QMenu* menu = new QMenu(parent);
        menu->addAction(QString::fromLocal8Bit("椭圆形状"));
        menu->addAction(QString::fromLocal8Bit("旋转"));
        return menu;
    }
};

//*******************************************************************************************/
//工厂
//*******************************************************************************************/
// 菜单策略工厂注册器
class MenuStrategyFactory {
public:
    using Creator = std::function<std::shared_ptr<MenuStrategy>()>;

    static MenuStrategyFactory& instance() {
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
//图元
//*******************************************************************************************/
class BaseCustomItem : public QGraphicsItem {
public:
    BaseCustomItem() = default;
    virtual ~BaseCustomItem() = default;

    // 每个派生类必须重写返回自己的类型标识，用于菜单策略工厂匹配
    virtual QString objectType() const = 0;

    void initMenuStrategy() {   
        menuStrategy = MenuStrategyFactory::instance().create(objectType());
    }

    // 设置菜单策略
    void setMenuStrategy(std::shared_ptr<MenuStrategy> strategy) {
        menuStrategy = std::move(strategy);
    }

    // 图元基类中不需要了，因为已经被scene代理且分发了
    // 右键菜单事件统一实现
    // void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override {
    //     if (menuStrategy) {
    //         QMenu* menu = menuStrategy->createMenu(nullptr);
    //         menu->exec(event->screenPos());
    //         delete menu;
    //     }
    // }

protected:
    std::shared_ptr<MenuStrategy> menuStrategy;
};

class CustomItem : public BaseCustomItem {
public:
    CustomItem() {
        initMenuStrategy();
    }

    QRectF boundingRect() const override {
        return QRectF(0, 0, 100, 50);
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override {
        painter->setPen(QColor(70, 130, 180));
        // painter->setBrush(QColor(70, 130, 180)); // 不填充
        painter->drawRect(boundingRect());
        painter->drawText(QPoint(10, 30), "TextItem");
    }

    QString objectType() const override {
        return "TextItem";  // 这里填对应类型字符串，方便工厂查找
    }
};

class CustomItem2 : public BaseCustomItem {
public:
    CustomItem2() {
        initMenuStrategy();
    }

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
    CustomItem3() {
        initMenuStrategy();
    }

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
                auto strategy = MenuStrategyFactory::instance().create(baseItem->objectType());
                if (strategy) {
                    QMenu* menu = strategy->createMenu(nullptr);
                    menu->exec(event->screenPos());
                    delete menu;
                    return;  // 菜单弹出后返回，防止继续冒泡
                }
            }
        }

        auto defaultStrategy = MenuStrategyFactory::instance().create("Background");
        if (defaultStrategy) {
            QMenu* menu = defaultStrategy->createMenu(nullptr);
            menu->exec(event->screenPos());
            delete menu;
            return;
        }

        QGraphicsScene::contextMenuEvent(event);

        // the old logic, now is replaced by the new logic, which is proxied to the item
        // if (!item && menuStrategy) {
        //     QMenu* menu = menuStrategy->createMenu(nullptr);
        //     menu->exec(event->screenPos());
        //     delete menu;
        // } else {
        //     QGraphicsScene::contextMenuEvent(event);
        // }
    }

private:
    std::shared_ptr<MenuStrategy> menuStrategy;
};

//*******************************************************************************************/
//注册
//*******************************************************************************************/
// 注册各种策略
void registerMenuStrategies() {
    MenuStrategyFactory::instance().registerCreator("TextItem", []() {
        // 装饰基础菜单
        return std::make_shared<BaseMenuDecorator>(std::make_shared<TextItemMenuStrategy>());
    });
    MenuStrategyFactory::instance().registerCreator("Background", []() {
        return std::make_shared<BaseMenuDecorator>(std::make_shared<BackgroundMenuStrategy>());
    });
    MenuStrategyFactory::instance().registerCreator("Special", []() {
        // 不装饰基础菜单，只有特殊菜单
        return std::make_shared<NoBaseMenuStrategy>();
    });
    MenuStrategyFactory::instance().registerCreator("Circle", []() {
        return std::make_shared<BaseMenuDecorator>(std::make_shared<CircleMenuStrategy>());
    });
}



int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    registerMenuStrategies();

    // 使用工厂创建策略
    // auto textStrategy = MenuStrategyFactory::instance().create("TextItem");
    // auto bgStrategy = MenuStrategyFactory::instance().create("Background");
    // auto specialStrategy = MenuStrategyFactory::instance().create("Special");
    // auto circleStrategy = MenuStrategyFactory::instance().create("Circle");

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
