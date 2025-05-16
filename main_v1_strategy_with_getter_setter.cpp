// main.cpp
#include <QApplication>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>
#include <memory>
#include <QGraphicsSceneEvent>

// Strategy interface
class MenuStrategy {
public:
    virtual ~MenuStrategy() = default;
    virtual QMenu* createMenu(QWidget* parent) = 0;
};

// Concrete strategy for text item
class TextItemMenuStrategy : public MenuStrategy {
public:
    QMenu* createMenu(QWidget* parent) override {
        QMenu* menu = new QMenu(parent);
        menu->addAction("Edit Text");
        menu->addAction("Change Font");
        return menu;
    }
};

// Concrete strategy for background
class BackgroundMenuStrategy : public MenuStrategy {
public:
    QMenu* createMenu(QWidget* parent) override {
        QMenu* menu = new QMenu(parent);
        menu->addAction("Add Slide");
        menu->addAction("Slide Properties");
        return menu;
    }
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

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    std::shared_ptr<MenuStrategy> textStrategy = std::make_shared<TextItemMenuStrategy>();
    std::shared_ptr<MenuStrategy> bgStrategy = std::make_shared<BackgroundMenuStrategy>();

    CustomScene* scene = new CustomScene(bgStrategy);
    CustomItem* item = new CustomItem(textStrategy);
    item->setPos(50, 50);
    scene->addItem(item);

    QGraphicsView* view = new QGraphicsView(scene);
    view->setSceneRect(0, 0, 400, 300);
    view->show();

    return app.exec();
}
