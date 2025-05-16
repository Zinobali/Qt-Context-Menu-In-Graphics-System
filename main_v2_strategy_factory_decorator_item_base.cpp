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
        menu->addAction("Copy");
        menu->addAction("Cut");
        menu->addAction("Paste");
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
        menu->addAction("Edit Text");
        menu->addAction("Change Font");
        return menu;
    }
};

// Concrete strategy for background (基础菜单 + 特殊菜单)
class BackgroundMenuStrategy : public MenuStrategy {
public:
    QMenu* createMenu(QWidget* parent) override {
        QMenu* menu = new QMenu(parent);
        menu->addAction("Add Slide");
        menu->addAction("Slide Properties");
        return menu;
    }
};

// 不支持基础菜单，只显示自己的菜单
class NoBaseMenuStrategy : public MenuStrategy {
public:
    QMenu* createMenu(QWidget* parent) override {
        QMenu* menu = new QMenu(parent);
        menu->addAction("Only Special Action");
        return menu;
    }
};

// Concrete strategy for Circle (基础菜单 + 特殊菜单)
class CircleMenuStrategy : public MenuStrategy {
public:
    QMenu* createMenu(QWidget* parent) override {
        QMenu* menu = new QMenu(parent);
        menu->addAction("Circle Action");
        menu->addAction("Spin");
        return menu;
    }
};

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

// Custom QGraphicsItem
class CustomItem : public QGraphicsItem {
public:
    CustomItem(std::shared_ptr<MenuStrategy> strategy)
        : menuStrategy(std::move(strategy)) {}

    QRectF boundingRect() const override {
        return QRectF(0, 0, 100, 50);
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override {
        painter->setPen(QColor(70, 130, 180));
        painter->setBrush(QColor(70, 130, 180));
        painter->drawRect(boundingRect());
    }

    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override {
        if (menuStrategy) {
            QMenu* menu = menuStrategy->createMenu(nullptr);
            menu->exec(event->screenPos());
            delete menu;
        }
    }

    void setMenuStrategy(std::shared_ptr<MenuStrategy> strategy) {
        menuStrategy = std::move(strategy);
    }

private:
    std::shared_ptr<MenuStrategy> menuStrategy;
};

// Custom QGraphicsItem2
class CustomItem2 : public QGraphicsItem {
public:
    CustomItem2(std::shared_ptr<MenuStrategy> strategy)
        : menuStrategy(std::move(strategy)) {}

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

    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override {
        if (menuStrategy) {
            QMenu* menu = menuStrategy->createMenu(nullptr);
            menu->exec(event->screenPos());
            delete menu;
        }
    }

private:
    std::shared_ptr<MenuStrategy> menuStrategy;
};

// Custom QGraphicsScene
class CustomScene : public QGraphicsScene {
public:
    CustomScene(std::shared_ptr<MenuStrategy> strategy)
        : menuStrategy(std::move(strategy)) {}

protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override {
        QGraphicsItem* item = itemAt(event->scenePos(), QTransform());
        if (!item && menuStrategy) {
            QMenu* menu = menuStrategy->createMenu(nullptr);
            menu->exec(event->screenPos());
            delete menu;
        } else {
            QGraphicsScene::contextMenuEvent(event);
        }
    }

private:
    std::shared_ptr<MenuStrategy> menuStrategy;
};

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
    auto textStrategy = MenuStrategyFactory::instance().create("TextItem");
    auto bgStrategy = MenuStrategyFactory::instance().create("Background");
    auto specialStrategy = MenuStrategyFactory::instance().create("Special");
    auto circleStrategy = MenuStrategyFactory::instance().create("Circle");

    CustomScene* scene = new CustomScene(bgStrategy);
    // 添加带基础菜单的元素
    CustomItem* item1 = new CustomItem(textStrategy);
    item1->setPos(50, 50);
    scene->addItem(item1);

    // 添加不支持基础菜单的元素
    CustomItem* item2 = new CustomItem(specialStrategy);
    item2->setPos(200, 50);
    scene->addItem(item2);

    CustomItem2* item3 = new CustomItem2(circleStrategy);
    item3->setPos(100, 150);
    scene->addItem(item3);

    QGraphicsView* view = new QGraphicsView(scene);
    view->setSceneRect(0, 0, 400, 300);
    view->show();

    return app.exec();
}
