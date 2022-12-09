#include "mainwindow-dtk.h"
#include "ui_mainwindow-dtk.h"

#include <DWidgetUtil>
#include <DGuiApplicationHelper>
#include <QSizePolicy>

MainWindow::MainWindow(QWidget *parent)
    : DBlurEffectWidget(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    moveToCenter(this); //让窗口居中显示

    downloadlistwidget = new DownloadListWidget;
    downloadButton = new QPushButton(ui->titlebar);
    backButtom = new QPushButton(ui->titlebar);
    ui->appintopage->setDownloadWidget(downloadlistwidget);
    ui->stackedWidget->setCurrentIndex(0);
    ui->titlebar->setBackgroundTransparent(true);
    //ui->titlebar->setSwitchThemeMenuVisible(false); // 去除 dtk 标题栏主题切换菜单
    setMaskAlpha(240);

    initConfig();
    updateUi(0);

    //主题切换
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, [=](DGuiApplicationHelper::ColorType themeType) {
        if (themeType == DGuiApplicationHelper::DarkType) {
            //深色模式
            setMaskColor(QColor("#2a2b2b"));
            this->setStyleSheet("#mainpage{background-color: transparent;border-radius:14px;}\
                                QLabel#cardtitle,QLabel#title,QLabel#title_1,QLabel#title_2,QLabel#title_3 {color:#FFFFFF}\
                                ");
            backButtom->setIcon(QIcon(":/icon/dark/back.svg"));
            downloadButton->setIcon(QIcon(":/icon/dark/download.svg"));
            int i = 0;
            while (i < ui->buttonGroup->buttons().size()) {
                ui->buttonGroup->buttons()[i]->setIcon(QIcon(":/icon/dark/leftbutton_" + QString::number(i) + ".svg"));
                if (QLocale::system().name() == "zh_CN") {
                    ui->buttonGroup->buttons()[i]->setStyleSheet("QPushButton{background-color:transparent;}\
                                                                  QPushButton:hover{background-color:#7a7a7a;border:0px;border-radius:8px;}\
                                                                  QPushButton:checked{background-color:#6e6e6e;border:0px;border-radius:8px;}");
                } else {
                    ui->buttonGroup->buttons()[i]->setStyleSheet("QPushButton{background-color:transparent;text-align: left; padding-left: 15px;}\
                                                                  QPushButton:hover{background-color:#7a7a7a;border:0px;border-radius:8px;text-align: left; padding-left: 15px;}\
                                                                  QPushButton:checked{background-color:#6e6e6e;border:0px;border-radius:8px;text-align: left; padding-left: 15px;}");
                }
                i++;
            }
        } else {
            //亮色模式
            setMaskColor(QColor("#f3f7f8"));
            this->setStyleSheet("#mainpage{background-color: transparent;border-radius:14px;}\
                                QLabel#cardtitle,QLabel#title,QLabel#title_1,QLabel#title_2,QLabel#title_3 {color:#000000}\
                                ");
            backButtom->setIcon(QIcon(":/icon/light/back.svg"));
            downloadButton->setIcon(QIcon(":/icon/light/download.svg"));
            int i = 0;
            while (i < ui->buttonGroup->buttons().size()) {
                ui->buttonGroup->buttons()[i]->setIcon(QIcon(":/icon/light/leftbutton_" + QString::number(i) + ".svg"));
                if (QLocale::system().name() == "zh_CN") {
                    ui->buttonGroup->buttons()[i]->setStyleSheet("QPushButton{background-color:transparent;}\
                                                                  QPushButton:hover{background-color:#eAeAeA;border:0px;border-radius:8px;}\
                                                                  QPushButton:checked{background-color:#dddddd;border:0px;border-radius:8px;}");
                } else {
                    ui->buttonGroup->buttons()[i]->setStyleSheet("QPushButton{background-color:transparent;text-align: left; padding-left: 15px;}\
                                                                  QPushButton:hover{background-color:#eAeAeA;border:0px;border-radius:8px;text-align: left; padding-left: 15px;}\
                                                                  QPushButton:checked{background-color:#dddddd;border:0px;border-radius:8px;text-align: left; padding-left: 15px;}");
                }
                i++;
            }
        }
        ui->applistpage->setTheme(themeType == DGuiApplicationHelper::DarkType);
        ui->applistpage_1->setTheme(themeType == DGuiApplicationHelper::DarkType);
        ui->appintopage->setTheme(themeType == DGuiApplicationHelper::DarkType);
    });

    //初始化标题栏控件
    connect(downloadButton, &QPushButton::clicked, [=]() {
        QPoint pos;
        pos.setX(downloadButton->mapToGlobal(QPoint(0, 0)).x() + downloadButton->width() / 2 - downloadlistwidget->width() / 2);
        pos.setY(downloadButton->mapToGlobal(QPoint(0, 0)).y() + downloadButton->height() + 5);
        downloadlistwidget->m_move(pos.x(), pos.y());
        downloadlistwidget->show();
    });

    connect(backButtom, &QPushButton::clicked, [=]() {
        ui->stackedWidget->setCurrentIndex(pageHistory.at(pageHistory.count() - 2));
        if (pageHistory.at(pageHistory.count() - 1) == 3) {
            ui->appintopage->clear();
        }

        pageHistory.removeLast();
        if (pageHistory.count() > 1) {
            backButtom->show();
        } else {
            backButtom->hide();
        }
    });
    downloadlistwidget->hide();
    backButtom->hide();
    ui->titlebar->setIcon(QIcon::fromTheme(":/icon/logo.svg"));
    QWidget *w_titlebar = new QWidget(ui->titlebar);
    QHBoxLayout *ly_titlebar = new QHBoxLayout(w_titlebar);
    QLabel *title = new QLabel(this);
    title->setText(tr("Spark Store"));
    searchEdit->setPlaceholderText(tr("Search or enter spk://"));
    ly_titlebar->addWidget(title);
    ly_titlebar->addWidget(backButtom);
    ly_titlebar->addStretch(2.5);
    ly_titlebar->addWidget(searchEdit);
    ly_titlebar->addWidget(downloadButton);
    ly_titlebar->addStretch(1);
    ui->titlebar->setCustomWidget(w_titlebar);
    //侧边栏按钮
    int i = 0;
    while (i < ui->buttonGroup->buttons().size()) {
        ui->buttonGroup->buttons()[i]->setStyleSheet("QPushButton{qproperty-icon: url(data/images/userMangaer/teacher.png);}");
        connect(ui->buttonGroup->buttons()[i], &QPushButton::toggled, [=](bool checked) {
            if (checked == true) {
                ui->appintopage->clear();
                updateUi(i);
            }
        });
        i++;
    }

    // 搜索事件
    connect(searchEdit, &DSearchEdit::returnPressed, this, [=]() {
        QString searchtext = searchEdit->text();
        if (!searchtext.isEmpty()) {
            if (searchtext.startsWith("spk://")) {
                openUrl(QUrl(searchtext));
                searchEdit->clearEdit();
            } else {
                ui->applistpage_1->getSearchList(searchtext);
                switchPage(2);
                searchEdit->clearEdit();
            }
        }
        this->setFocus();
    });

    // 列表点击事件
    connect(ui->applistpage, &AppListPage::clicked, this, [=](QUrl spk) {
        openUrl(spk);
    });
    connect(ui->applistpage_1, &AppListPage::clicked, this, [=](QUrl spk) {
        openUrl(spk);
    });

    emit DGuiApplicationHelper::instance()->themeTypeChanged(DGuiApplicationHelper::instance()->themeType());
}

MainWindow::~MainWindow()
{
    delete searchEdit;
    delete downloadlistwidget;
    delete ui;
}

void MainWindow::openUrl(QUrl url)
{
    if (url.toString().startsWith("spk://")) {
        ui->appintopage->openUrl(url);
        switchPage(3);
    } else {
        QDesktopServices::openUrl(url);
    }
}

void MainWindow::initConfig()
{
    // 新建临时文件夹
    QDir dir;
    dir.mkpath("/tmp/spark-store");
}

void MainWindow::switchPage(int now) //临时方案，回家后修改
{
    qDebug() << pageHistory.count();
    if (pageHistory.count() >= 1) {
        backButtom->show();
    } else {
        backButtom->hide();
    }
    ui->stackedWidget->setCurrentIndex(now);
    pageHistory << now;
}

//刷新界面
void MainWindow::updateUi(int now)
{
    pageHistory.clear();
        QStringList itemlist;
        itemlist << ""
                 << "network"
                 << "relations"
                 << "musicandsound"
                 << "videos"
                 << "photos"
                 << "games"
                 << "office"
                 << "reading"
                 << "programming"
                 << "tools"
                 << "themes"
                 << "others";
        ui->applistpage->getAppList(itemlist[now]);
        qDebug() << itemlist[now];
        switchPage(1);
}
