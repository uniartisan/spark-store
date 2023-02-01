#include "mainwindow-dtk.h"
#include "ui_mainwindow-dtk.h"
#include "utils/widgetanimation.h"
#include "widgets/common/progressbutton.h"
#include "widgets/downloadlistwidget.h"
#include "dbus/dbussparkstoreservice.h"
#include "application.h"

#include <DLabel>
#include <DWidgetUtil>
#include <DGuiApplicationHelper>

#include <QAbstractButton>
#include <QtConcurrent>

#define AppPageApplist 0
#define AppPageSearchlist 1
#define AppPageAppdetail 2
#define AppPageSettings 3
#define WaylandSearchCenter 1
#define OtherSearchCenter 2
#define RightSearchSpace 1
#define UploadServerUrl "https://upload.deepinos.org/"

MainWindow::MainWindow(QWidget *parent)
    : BaseWidgetOpacity(parent)
    , ui(new Ui::MainWindow)
    , downloadlistwidget(new DownloadListWidget)
    , trayIcon(new QSystemTrayIcon(QIcon::fromTheme("spark-store"), this))
{
    ui->setupUi(this);

    initTmpDir();

    initUI();
    initConnections();

    ui->appintopage->setDownloadWidget(downloadlistwidget);

    emit DGuiApplicationHelper::instance()->themeTypeChanged(DGuiApplicationHelper::instance()->themeType());
}

MainWindow::~MainWindow()
{
    delete ui;

    downloadlistwidget->deleteLater();
}

void MainWindow::initDbus()
{
    DBusSparkStoreService *dbusInter = new DBusSparkStoreService(this);

    QDBusConnection::sessionBus().registerService("com.gitee.spark.store");
    QDBusConnection::sessionBus().registerObject("/com/gitee/spark/store", "com.gitee.spark.store", this);
    connect(dbusInter, &DBusSparkStoreService::sigOpenUrl, this, &MainWindow::onGetUrl);
}

void MainWindow::onGetUrl(const QString &url)
{
    if (url.trimmed().startsWith("spk://"))
    {
        openUrl(url);
    }

    showWindowAnimation = false;
    closeWindowAnimation = false;

    setWindowState(windowState() & Qt::WindowActive);
    activateWindow();
    show();
}

void MainWindow::onNewProcessInstance(qint64 pid, const QStringList &arguments)
{
    Q_UNUSED(pid)

    onGetUrl(arguments.value(1, ""));
}

void MainWindow::openUrl(const QString &url)
{
    if (url.startsWith("spk://search/"))
    {
        QString keyword = url.mid(13);
        if (keyword == "%")
        {
            qWarning() << "keyword '%' is not valid, which will cause QtWebEngine crash.";
            return;
        }
        ui->applistpage_1->getSearchList(keyword);
        switchPage(AppPageSearchlist);
    }
    else if (url.startsWith("spk://"))
    {
        ui->appintopage->openUrl(QUrl::fromUserInput(url));
        switchPage(AppPageAppdetail);
    }
    else
    {
        QDesktopServices::openUrl(QUrl::fromUserInput(url));
    }
}

void MainWindow::initUI()
{
    setWindowTitle(QObject::tr("Spark Store"));
    setMaskAlpha(250);

    initTitleBar();
    initLeftMenu();

    ui->stackedWidget->setCurrentIndex(0);
    updateUi(0);

    initTrayIcon();
}

void MainWindow::initTitleBar()
{
    ui->titlebar->setIcon(QIcon::fromTheme("spark-store"));
    ui->titlebar->setBackgroundTransparent(true);
    // ui->titlebar->setSwitchThemeMenuVisible(false); // 去除 dtk 标题栏主题切换菜单

    // 初始化标题栏控件
    DLabel *title = new DLabel(ui->titlebar);
    title->setText(tr("Spark Store"));

    backButton = new DPushButton(ui->titlebar);

    searchEdit = new DSearchEdit(ui->titlebar);
    searchEdit->setPlaceholderText(tr("Search or enter spk://"));

    downloadButton = new ProgressButton(ui->titlebar);

    QWidget *w_titlebar = new QWidget(ui->titlebar);
    QHBoxLayout *ly_titlebar = new QHBoxLayout(w_titlebar);
    ly_titlebar->addWidget(title);
    ly_titlebar->addWidget(backButton);
    // Check wayland configs
    QSettings config(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/config.ini", QSettings::IniFormat);
    if (!config.value("build/isDeepinOS").toBool() && config.value("build/useWayland").toBool())
    {
        // Wayland 搜索栏居中
        ly_titlebar->addStretch(WaylandSearchCenter);
    }
    else
    {
        // dwayland dxcb 搜索栏顶部右侧居中
        ly_titlebar->addStretch(OtherSearchCenter);
    }
    ly_titlebar->addWidget(searchEdit);
    ly_titlebar->addWidget(downloadButton);
    ly_titlebar->addStretch(RightSearchSpace);
    ui->titlebar->setCustomWidget(w_titlebar);

    initTitleBarMenu();

    backButton->hide();
    downloadlistwidget->hide();
}

void MainWindow::initTitleBarMenu()
{
    QAction *actionSubmission = new QAction(tr("Submit App"), this);
    QAction *actionSubmissionWithClient = new QAction(tr("Submit App with client(Recommanded)"), this);
    QAction *setting = new QAction(tr("Settings"));
    QAction *upgrade = new QAction(tr("APP Upgrade and Install Settings"));
    QMenu *menu = new QMenu(ui->titlebar);
    menu->addAction(setting);
    menu->addAction(upgrade);
    menu->addAction(actionSubmission);
    menu->addAction(actionSubmissionWithClient);

    ui->titlebar->setMenu(menu);

    connect(actionSubmission, &QAction::triggered, this, [=]
            { QDesktopServices::openUrl(QUrl(UploadServerUrl)); });
    connect(setting, &QAction::triggered, this, [=]
            {
        switchPage(AppPageSettings);
        ui->settingspage->updateUI(); });
    connect(upgrade, &QAction::triggered, this, [=]
            { QProcess::startDetached("/opt/durapps/spark-store/bin/update-upgrade/ss-update-controler.sh", QStringList()); });
    // 投稿器
    connect(actionSubmissionWithClient, &QAction::triggered, this, [=]
            {
                QString submitterSpk = "spk://store/tools/spark-store-submitter";
                QFile actionSubmissionClientStatus("/opt/spark-store-submitter/bin/spark-store-submitter");
                if (actionSubmissionClientStatus.exists())
                {
                    qDebug() << "投稿器存在";
                    QProcess::startDetached("/opt/spark-store-submitter/bin/spark-store-submitter", QStringList());
                }
                else
                {
                    qDebug() << "投稿器不存在，跳转页面";
                    openUrl(submitterSpk);
                } });
}

void MainWindow::initLeftMenu()
{
    // 侧边栏按钮
    foreach (QAbstractButton *button, ui->buttonGroup->buttons())
    {
        button->setStyleSheet("QPushButton{qproperty-icon: url(data/images/userMangaer/teacher.png);}");
        connect(button, &QPushButton::toggled, [=](bool checked)
                {
            if (checked == true) {
                searchEdit->clearEdit();
                updateUi(ui->buttonGroup->buttons().indexOf(button));
            } });
    }
}

void MainWindow::initTrayIcon()
{
    QMenu *menu = new QMenu(this);

    QAction *showAction = new QAction(QObject::tr("Show MainWindow"), menu);
    QAction *aboutAction = new QAction(qApp->translate("TitleBarMenu", "About"), menu);
    QAction *exitAction = new QAction(qApp->translate("TitleBarMenu", "Exit"), menu);
    menu->addAction(showAction);
    menu->addAction(aboutAction);
    menu->addAction(exitAction);
    trayIcon->setContextMenu(menu);

    connect(showAction, &QAction::triggered, this, [=]()
            {
        showWindowAnimation = false;
        closeWindowAnimation = false;

        setWindowState(windowState() & Qt::WindowActive);
        activateWindow();
        show(); });
    connect(aboutAction, &QAction::triggered, this, [=]()
            {
        qobject_cast<Application *>(qApp)->handleAboutAction(); });
    connect(exitAction, &QAction::triggered, this, [=]()
            {
        qobject_cast<Application *>(qApp)->quit(); });

    trayIcon->show();
}

void MainWindow::initConnections()
{
    // 主题切换
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, [=](DGuiApplicationHelper::ColorType themeType)
            {
        if (themeType == DGuiApplicationHelper::DarkType) {
            //深色模式
            setMaskColor(QColor("#2a2b2b"));
            backButton->setIcon(QIcon(":/icon/dark/back.svg"));
            downloadButton->setIcon(":/icon/dark/download.svg");
            downloadButton->setBackgroundColor(QColor("#444444"));
            downloadButton->setColor(QColor("#66CCFF"));
            ui->pushButton_14->setIcon(QIcon(":/icon/dark/update.svg"));
            for (int i = 0; i < ui->buttonGroup->buttons().size(); i++) {
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
            }
        } else {
            //亮色模式
            setMaskColor(QColor("#f3f7f8"));
            backButton->setIcon(QIcon(":/icon/light/back.svg"));
            downloadButton->setBackgroundColor(QColor("#e3e4e4"));
            downloadButton->setColor(QColor("#66CCFF"));
            downloadButton->setIcon(":/icon/light/download.svg");
            ui->pushButton_14->setIcon(QIcon(":/icon/light/update.svg"));
            for (int i = 0; i < ui->buttonGroup->buttons().size(); i++) {
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
            }
        }
        ui->pushButton_14->setStyleSheet(ui->pushButton_4->styleSheet());
        ui->applistpage->setTheme(themeType == DGuiApplicationHelper::DarkType);
        ui->applistpage_1->setTheme(themeType == DGuiApplicationHelper::DarkType);
        ui->appintopage->setTheme(themeType == DGuiApplicationHelper::DarkType);
        ui->settingspage->setTheme(themeType == DGuiApplicationHelper::DarkType); });

    connect(downloadButton, &ProgressButton::clicked, [=]()
            {
        QPoint pos;
        pos.setX(downloadButton->mapToGlobal(QPoint(0, 0)).x() + downloadButton->width() / 2 - downloadlistwidget->width() / 2);
        pos.setY(downloadButton->mapToGlobal(QPoint(0, 0)).y() + downloadButton->height() + 5);
        downloadlistwidget->m_move(pos.x(), pos.y());
        downloadlistwidget->show(); });

    // appintopage按下下载按钮时标题栏下载列表按钮抖动
    connect(ui->appintopage, &AppIntoPage::clickedDownloadBtn, [=]()
            {
                WidgetAnimation::widgetShake(downloadButton, 6); // 第一个参数是抖动的控件，第二个参数是抖动范围（像素）
            });

    connect(backButton, &QPushButton::clicked, [=]()
            {
        ui->stackedWidget->setCurrentIndex(pageHistory.at(pageHistory.count() - 2));
        pageHistory.removeLast();
        if (pageHistory.count() > 1) {
            backButton->show();
        } else {
            backButton->hide();
        } });

    // 搜索事件
    connect(searchEdit, &DSearchEdit::returnPressed, this, [=]()
            {
        QString searchtext = searchEdit->text();
        if (!searchtext.isEmpty()) {
            if (searchtext.startsWith("spk://")) {
                openUrl(searchtext);
                searchEdit->clearEdit();
            } else {
                if (searchtext == "%")
                {
                    qWarning() << "keyword '%' matches too many results, which will cause QtWebEngine crash.";
                }
                else
                {
                    ui->applistpage_1->getSearchList(searchtext);
                    switchPage(AppPageSearchlist);
                }
            }
        }
        this->setFocus(); });

    connect(downloadlistwidget, &DownloadListWidget::downloadProgress, this, [=](int i)
            { downloadButton->setProgress(i); });
    // 列表点击事件
    connect(ui->applistpage, &AppListPage::clicked, this, [=](QUrl spk)
            { openUrl(spk.toString()); });
    connect(ui->applistpage_1, &AppListPage::clicked, this, [=](QUrl spk)
            { openUrl(spk.toString()); });
    connect(ui->settingspage, &SettingsPage::openUrl, this, [=](QUrl spk)
            { openUrl(spk.toString()); });

    // 托盘图标点击事件
    connect(trayIcon, &QSystemTrayIcon::activated, this, [=](QSystemTrayIcon::ActivationReason reason)
            {
        switch (reason) {
        case QSystemTrayIcon::Trigger:
        {
            showWindowAnimation = false;
            closeWindowAnimation = false;

            setWindowState(windowState() & Qt::WindowActive);
            activateWindow();
            show();

            Q_FALLTHROUGH();
        }
        default:
            break;
        } });

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::newProcessInstance, this, &MainWindow::onNewProcessInstance);
}

void MainWindow::initTmpDir()
{
    // 新建临时文件夹
    QDir dir;
    dir.mkpath("/tmp/spark-store");
}

void MainWindow::switchPage(int now) // 临时方案，回家后修改
{
    qDebug() << pageHistory.count();
    if (pageHistory.count() >= 1)
    {
        backButton->show();
    }
    else
    {
        backButton->hide();
    }
    ui->stackedWidget->setCurrentIndex(now);
    ui->stackedWidget->currentWidget()->setFocus();
    pageHistory << now;
}

// 刷新界面
void MainWindow::updateUi(int now)
{
    pageHistory.clear();
    QStringList itemlist;
    itemlist << ""
             << "network"
             << "chat"
             << "music"
             << "video"
             << "image_graphics"
             << "games"
             << "office"
             << "reading"
             << "development"
             << "tools"
             << "themes"
             << "others";
    ui->applistpage->getAppList(itemlist[now]);
    qDebug() << itemlist[now];
    switchPage(AppPageApplist);
}

void MainWindow::on_pushButton_14_clicked()
{
    // Check UOS
    QSettings config(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/config.ini", QSettings::IniFormat);
    if (config.contains("UOS/EnableDeveloperMode") && !config.value("UOS/EnableDeveloperMode").toBool())
    {
        qDebug() << "UOS Developer Mode has not been enabled!";
        QtConcurrent::run([=]
                          {
            auto upgradeP = new QProcess();
            upgradeP->startDetached("zenity", QStringList() << "--info"
                                                            << "--text"
                                                            << "UOS开发者模式未开启，相关功能被禁用"
                                                            << "--title"
                                                            << "功能禁用提示"
                                                            << "--width"
                                                            << "360"
                                                            );
            upgradeP->waitForStarted();
            upgradeP->waitForFinished(30);
            upgradeP->deleteLater(); });
    }
    else
    {
        QFile upgradeStatus("/tmp/spark-store/upgradeStatus.txt");
        if (!upgradeStatus.exists())
        {
            QtConcurrent::run([=]
                              {
            auto upgradeP = new QProcess();
            upgradeP->startDetached("/opt/durapps/spark-store/bin/update-upgrade/ss-do-upgrade.sh", QStringList());
            upgradeP->waitForStarted();
            upgradeP->waitForFinished(-1);
            upgradeP->deleteLater(); });
        }
    }
}
