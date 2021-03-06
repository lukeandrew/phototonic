/*
 *  Copyright (C) 2013-2014 Ofer Kashayov <oferkv@live.com>
 *  This file is part of Phototonic Image Viewer.
 *
 *  Phototonic is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Phototonic is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Phototonic.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mainwindow.h"
#include "global.h"

#define THUMB_SIZE_MIN	50
#define THUMB_SIZE_MAX	300

Phototonic::Phototonic(QWidget *parent) : QMainWindow(parent)
{
	GData::appSettings = new QSettings("phototonic", "phototonic_103");
	readSettings();
	createThumbView();
	createActions();
	createMenus();
	createToolBars();
	createStatusBar();
	createFSTree();
	createBookmarks();
	createImageView();
	updateExternalApps();
	loadShortcuts();
	setupDocks();

	connect(qApp, SIGNAL(focusChanged(QWidget*, QWidget*)), 
				this, SLOT(updateActions(QWidget*, QWidget*)));

	restoreGeometry(GData::appSettings->value("Geometry").toByteArray());
	restoreState(GData::appSettings->value("WindowState").toByteArray());
	setWindowIcon(QIcon(":/images/phototonic.png"));

	mainLayout = new QHBoxLayout;
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);
	mainLayout->addWidget(thumbView);
	QWidget *centralWidget = new QWidget;
	centralWidget->setLayout(mainLayout);
	setCentralWidget(centralWidget);

	handleStartupArgs();

	copyMoveToDialog = 0;
	colorsDialog = 0;
	cropDialog = 0;
	initComplete = true;
	thumbView->busy = false;
	currentHistoryIdx = -1;
	needHistoryRecord = true;
	interfaceDisabled = false;

	refreshThumbs(true);
	if (GData::layoutMode == thumbViewIdx)
		thumbView->setFocus(Qt::OtherFocusReason);
	if (!cliImageLoaded)
		QTimer::singleShot(100, this, SLOT(selectRecentThumb()));
	else 
		QTimer::singleShot(100, this, SLOT(updateIndexByViewerImage()));
}

void Phototonic::handleStartupArgs()
{
	cliImageLoaded = false;
	if (QCoreApplication::arguments().size() == 2)
	{
		QFileInfo cliArg(QCoreApplication::arguments().at(1));
		if (cliArg.isDir())
		{
			thumbView->currentViewDir = QCoreApplication::arguments().at(1);
			cliImageLoaded = false;
		}
		else
		{
			thumbView->currentViewDir = cliArg.absolutePath();
			cliFileName = thumbView->currentViewDir + QDir::separator() + cliArg.fileName();
			cliImageLoaded = true;
			loadImagefromCli();
		}
	}
	else
	{
		if (GData::startupDir == GData::specifiedDir)
			thumbView->currentViewDir = GData::specifiedStartDir;
		else if (GData::startupDir == GData::rememberLastDir)
			thumbView->currentViewDir = GData::appSettings->value("lastDir").toString();
	}
	selectCurrentViewDir();
}

bool Phototonic::event(QEvent *event)
{
	if (event->type() == QEvent::ActivationChange ||
			(GData::layoutMode == thumbViewIdx && event->type() == QEvent::MouseButtonRelease)) { 
		thumbView->loadVisibleThumbs();
	}
	
	return QMainWindow::event(event);
}

void Phototonic::createThumbView()
{
	thumbView = new ThumbView(this);
	thumbView->thumbsSortFlags = (QDir::SortFlags)GData::appSettings->value("thumbsSortFlags").toInt();
	thumbView->thumbsSortFlags |= QDir::IgnoreCase;

	connect(thumbView, SIGNAL(setStatus(QString)), this, SLOT(setStatus(QString)));
	connect(thumbView, SIGNAL(showBusy(bool)), this, SLOT(showBusyStatus(bool)));
	connect(thumbView->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), 
				this, SLOT(changeActionsBySelection(QItemSelection, QItemSelection)));

	iiDock = new QDockWidget(tr("Image Info"), this);
	iiDock->setObjectName("Image Info");
	iiDock->setWidget(thumbView->infoView);
	connect(iiDock->toggleViewAction(), SIGNAL(triggered()), this, SLOT(setIiDockVisibility()));	
	connect(iiDock, SIGNAL(visibilityChanged(bool)), this, SLOT(setIiDockVisibility()));	
}

void Phototonic::addMenuSeparator(QWidget *widget)
{
	QAction *separator = new QAction(this);
	separator->setSeparator(true);
	widget->addAction(separator);
}

void Phototonic::createImageView()
{
	imageView = new ImageView(this);
	connect(saveAction, SIGNAL(triggered()), imageView, SLOT(saveImage()));
	connect(saveAsAction, SIGNAL(triggered()), imageView, SLOT(saveImageAs()));
	connect(copyImageAction, SIGNAL(triggered()), imageView, SLOT(copyImage()));
	connect(pasteImageAction, SIGNAL(triggered()), imageView, SLOT(pasteImage()));
	connect(cropToSelectionAct, SIGNAL(triggered()), imageView, SLOT(cropToSelection()));
	imageView->ImagePopUpMenu = new QMenu();

	// Widget actions
	imageView->addAction(slideShowAction);
	imageView->addAction(nextImageAction);
	imageView->addAction(prevImageAction);
	imageView->addAction(firstImageAction);
	imageView->addAction(lastImageAction);
	imageView->addAction(randomImageAction);
	imageView->addAction(zoomInAct);
	imageView->addAction(zoomOutAct);
	imageView->addAction(origZoomAct);
	imageView->addAction(resetZoomAct);
	imageView->addAction(rotateRightAct);
	imageView->addAction(rotateLeftAct);
	imageView->addAction(freeRotateRightAct);
	imageView->addAction(freeRotateLeftAct);
	imageView->addAction(flipHAct);
	imageView->addAction(flipVAct);
	imageView->addAction(cropAct);
	imageView->addAction(cropToSelectionAct);
	imageView->addAction(resizeAct);
	imageView->addAction(saveAction);
	imageView->addAction(saveAsAction);
	imageView->addAction(copyImageAction);
	imageView->addAction(pasteImageAction);
	imageView->addAction(deleteAction);
	imageView->addAction(renameAction);
	imageView->addAction(closeImageAct);
	imageView->addAction(fullScreenAct);
	imageView->addAction(settingsAction);
	imageView->addAction(mirrorDisabledAct);
	imageView->addAction(mirrorDualAct);
	imageView->addAction(mirrorTripleAct);
	imageView->addAction(mirrorVDualAct);
	imageView->addAction(mirrorQuadAct);
	imageView->addAction(keepTransformAct);
	imageView->addAction(keepZoomAct);
	imageView->addAction(refreshAction);
	imageView->addAction(colorsAct);
	imageView->addAction(moveRightAct);
	imageView->addAction(moveLeftAct);
	imageView->addAction(moveUpAct);
	imageView->addAction(moveDownAct);
	imageView->addAction(showClipboardAction);
	imageView->addAction(copyToAction);
	imageView->addAction(moveToAction);
	imageView->addAction(resizeAct);
	imageView->addAction(openAction);
	imageView->addAction(exitAction);

	// Actions
	addMenuSeparator(imageView->ImagePopUpMenu);
	imageView->ImagePopUpMenu->addAction(nextImageAction);
	imageView->ImagePopUpMenu->addAction(prevImageAction);
	imageView->ImagePopUpMenu->addAction(firstImageAction);
	imageView->ImagePopUpMenu->addAction(lastImageAction);
	imageView->ImagePopUpMenu->addAction(randomImageAction);
	imageView->ImagePopUpMenu->addAction(slideShowAction);

	addMenuSeparator(imageView->ImagePopUpMenu);
	zoomSubMenu = new QMenu(tr("Zoom"));
	zoomSubMenuAct = new QAction(tr("Zoom"), this);
	zoomSubMenuAct->setIcon(QIcon::fromTheme("edit-find", QIcon(":/images/zoom.png")));
	zoomSubMenuAct->setMenu(zoomSubMenu);
	imageView->ImagePopUpMenu->addAction(zoomSubMenuAct);
	zoomSubMenu->addAction(zoomInAct);
	zoomSubMenu->addAction(zoomOutAct);
	zoomSubMenu->addAction(origZoomAct);
	zoomSubMenu->addAction(resetZoomAct);
	addMenuSeparator(zoomSubMenu);
	zoomSubMenu->addAction(keepZoomAct);

	MirroringSubMenu = new QMenu(tr("Mirroring"));
	mirrorSubMenuAct = new QAction(tr("Mirroring"), this);
	mirrorSubMenuAct->setMenu(MirroringSubMenu);
	mirroringGroup = new QActionGroup(this);
	mirroringGroup->addAction(mirrorDisabledAct);
	mirroringGroup->addAction(mirrorDualAct);
	mirroringGroup->addAction(mirrorTripleAct);
	mirroringGroup->addAction(mirrorVDualAct);
	mirroringGroup->addAction(mirrorQuadAct);
	MirroringSubMenu->addActions(mirroringGroup->actions());

	transformSubMenu = new QMenu(tr("Transform"));
	transformSubMenuAct = new QAction(tr("Transform"), this);
	transformSubMenuAct->setMenu(transformSubMenu);
	imageView->ImagePopUpMenu->addAction(resizeAct);
	imageView->ImagePopUpMenu->addAction(cropToSelectionAct);
	imageView->ImagePopUpMenu->addAction(transformSubMenuAct);
	transformSubMenu->addAction(colorsAct);
	transformSubMenu->addAction(rotateRightAct);
	transformSubMenu->addAction(rotateLeftAct);
	transformSubMenu->addAction(freeRotateRightAct);
	transformSubMenu->addAction(freeRotateLeftAct);
	transformSubMenu->addAction(flipHAct);
	transformSubMenu->addAction(flipVAct);
	transformSubMenu->addAction(cropAct);

	addMenuSeparator(transformSubMenu);
	transformSubMenu->addAction(keepTransformAct);
	imageView->ImagePopUpMenu->addAction(mirrorSubMenuAct);

	addMenuSeparator(imageView->ImagePopUpMenu);
	imageView->ImagePopUpMenu->addAction(copyToAction);
	imageView->ImagePopUpMenu->addAction(moveToAction);
	imageView->ImagePopUpMenu->addAction(saveAction);
	imageView->ImagePopUpMenu->addAction(saveAsAction);
	imageView->ImagePopUpMenu->addAction(renameAction);
	imageView->ImagePopUpMenu->addAction(deleteAction);
	imageView->ImagePopUpMenu->addAction(openWithMenuAct);

	addMenuSeparator(imageView->ImagePopUpMenu);
	viewSubMenu = new QMenu(tr("View"));
	viewSubMenuAct = new QAction(tr("View"), this);
	viewSubMenuAct->setMenu(viewSubMenu);
	imageView->ImagePopUpMenu->addAction(viewSubMenuAct);
	viewSubMenu->addAction(fullScreenAct);
	viewSubMenu->addAction(showClipboardAction);
	viewSubMenu->addAction(actShowViewerToolbars);
	viewSubMenu->addAction(refreshAction);
	imageView->ImagePopUpMenu->addAction(copyImageAction);
	imageView->ImagePopUpMenu->addAction(pasteImageAction);
	imageView->ImagePopUpMenu->addAction(closeImageAct);
	imageView->ImagePopUpMenu->addAction(exitAction);

	addMenuSeparator(imageView->ImagePopUpMenu);
	imageView->ImagePopUpMenu->addAction(settingsAction);

	imageView->setContextMenuPolicy(Qt::DefaultContextMenu);
	GData::isFullScreen = GData::appSettings->value("isFullScreen").toBool();
	fullScreenAct->setChecked(GData::isFullScreen); 
}

void Phototonic::createActions()
{
	thumbsGoTopAct = new QAction(tr("Top"), this);
	thumbsGoTopAct->setIcon(QIcon::fromTheme("go-top", QIcon(":/images/top.png")));
	connect(thumbsGoTopAct, SIGNAL(triggered()), this, SLOT(goTop()));

	thumbsGoBottomAct = new QAction(tr("Bottom"), this);
	thumbsGoBottomAct->setIcon(QIcon::fromTheme("go-bottom", QIcon(":/images/bottom.png")));
	connect(thumbsGoBottomAct, SIGNAL(triggered()), this, SLOT(goBottom()));

	closeImageAct = new QAction(tr("Close Image"), this);
	connect(closeImageAct, SIGNAL(triggered()), this, SLOT(hideViewer()));

	fullScreenAct = new QAction(tr("Full Screen"), this);
	fullScreenAct->setCheckable(true);
	connect(fullScreenAct, SIGNAL(triggered()), this, SLOT(toggleFullScreen()));
	
	settingsAction = new QAction(tr("Preferences"), this);
	settingsAction->setIcon(QIcon::fromTheme("preferences-other", QIcon(":/images/settings.png")));
	connect(settingsAction, SIGNAL(triggered()), this, SLOT(showSettings()));

	exitAction = new QAction(tr("Exit"), this);
	connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));

	thumbsZoomInAct = new QAction(tr("Enlarge Thumbnails"), this);
	connect(thumbsZoomInAct, SIGNAL(triggered()), this, SLOT(thumbsZoomIn()));
	thumbsZoomInAct->setIcon(QIcon::fromTheme("zoom-in", QIcon(":/images/zoom_in.png")));
	if (thumbView->thumbSize == THUMB_SIZE_MAX)
		thumbsZoomInAct->setEnabled(false);

	thumbsZoomOutAct = new QAction(tr("Shrink Thumbnails"), this);
	connect(thumbsZoomOutAct, SIGNAL(triggered()), this, SLOT(thumbsZoomOut()));
	thumbsZoomOutAct->setIcon(QIcon::fromTheme("zoom-out", QIcon(":/images/zoom_out.png")));
	if (thumbView->thumbSize == THUMB_SIZE_MIN)
		thumbsZoomOutAct->setEnabled(false);

	cutAction = new QAction(tr("Cut"), this);
	cutAction->setIcon(QIcon::fromTheme("edit-cut", QIcon(":/images/cut.png")));
	connect(cutAction, SIGNAL(triggered()), this, SLOT(cutThumbs()));
	cutAction->setEnabled(false);

	copyAction = new QAction(tr("Copy"), this);
	copyAction->setIcon(QIcon::fromTheme("edit-copy", QIcon(":/images/copy.png")));
	connect(copyAction, SIGNAL(triggered()), this, SLOT(copyThumbs()));
	copyAction->setEnabled(false);

	copyToAction = new QAction(tr("Copy to..."), this);
	connect(copyToAction, SIGNAL(triggered()), this, SLOT(copyImagesTo()));

	moveToAction = new QAction(tr("Move to..."), this);
	connect(moveToAction, SIGNAL(triggered()), this, SLOT(moveImagesTo()));
	
	deleteAction = new QAction(tr("Delete"), this);
	deleteAction->setIcon(QIcon::fromTheme("edit-delete", QIcon(":/images/delete.png")));
	connect(deleteAction, SIGNAL(triggered()), this, SLOT(deleteOp()));

	saveAction = new QAction(tr("Save"), this);
	saveAction->setIcon(QIcon::fromTheme("document-save", QIcon(":/images/save.png")));

	saveAsAction = new QAction(tr("Save As"), this);
	saveAsAction->setIcon(QIcon::fromTheme("document-save-as", QIcon(":/images/save_as.png")));
	
	copyImageAction = new QAction(tr("Copy Image Data"), this);
	pasteImageAction = new QAction(tr("Paste Image Data"), this);

	renameAction = new QAction(tr("Rename"), this);
	connect(renameAction, SIGNAL(triggered()), this, SLOT(rename()));

	selectAllAction = new QAction(tr("Select All"), this);
	connect(selectAllAction, SIGNAL(triggered()), this, SLOT(selectAllThumbs()));

	aboutAction = new QAction(tr("About"), this);
	aboutAction->setIcon(QIcon::fromTheme("help-about", QIcon(":/images/about.png")));
	connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));

	// Sort actions
	actName = new QAction(tr("Name"), this);
	actTime = new QAction(tr("Time"), this);
	actSize = new QAction(tr("Size"), this);
	actType = new QAction(tr("Type"), this);
	actReverse = new QAction(tr("Reverse"), this);
	actName->setCheckable(true);
	actTime->setCheckable(true);
	actSize->setCheckable(true);
	actType->setCheckable(true);
	actReverse->setCheckable(true);
	connect(actName, SIGNAL(triggered()), this, SLOT(sortThumbnains()));
	connect(actTime, SIGNAL(triggered()), this, SLOT(sortThumbnains()));
	connect(actSize, SIGNAL(triggered()), this, SLOT(sortThumbnains()));
	connect(actType, SIGNAL(triggered()), this, SLOT(sortThumbnains()));
	connect(actReverse, SIGNAL(triggered()), this, SLOT(sortThumbnains()));

	if (thumbView->thumbsSortFlags & QDir::Time)
		actTime->setChecked(true);	
	else if (thumbView->thumbsSortFlags & QDir::Size)
		actSize->setChecked(true);	
	else if (thumbView->thumbsSortFlags & QDir::Type)
		actType->setChecked(true);	
	else
		actName->setChecked(true);	
	actReverse->setChecked(thumbView->thumbsSortFlags & QDir::Reversed); 

	actShowHidden = new QAction(tr("Show Hidden Files"), this);
	actShowHidden->setCheckable(true);
	actShowHidden->setChecked(GData::showHiddenFiles);
	connect(actShowHidden, SIGNAL(triggered()), this, SLOT(showHiddenFiles()));

	actShowLabels = new QAction(tr("Show Labels"), this);
	actShowLabels->setCheckable(true);
	actShowLabels->setChecked(GData::showLabels);
	connect(actShowLabels, SIGNAL(triggered()), this, SLOT(showLabels()));
	actShowLabels->setEnabled(GData::thumbsLayout != ThumbView::Squares)	;

	actSmallIcons = new QAction(tr("Small Icons"), this);
	actSmallIcons->setCheckable(true);
	actSmallIcons->setChecked(GData::smallIcons);
	connect(actSmallIcons, SIGNAL(triggered()), this, SLOT(setToolbarIconSize()));

	actLockDocks = new QAction(tr("Hide Docks Title Bar"), this);
	actLockDocks->setCheckable(true);
	actLockDocks->setChecked(GData::LockDocks);
	connect(actLockDocks, SIGNAL(triggered()), this, SLOT(lockDocks()));

	actShowViewerToolbars = new QAction(tr("Show Toolbar"), this);
	actShowViewerToolbars->setCheckable(true);
	actShowViewerToolbars->setChecked(GData::imageToolbarFullScreen);
	connect(actShowViewerToolbars, SIGNAL(triggered()), this, SLOT(toggleImageToolbar()));

	actClassic = new QAction(tr("Classic Thumbs"), this);
	actCompact = new QAction(tr("Compact"), this);
	actSquarish = new QAction(tr("Squarish"), this);
	connect(actClassic, SIGNAL(triggered()), this, SLOT(setClassicThumbs()));
	connect(actCompact, SIGNAL(triggered()), this, SLOT(setCompactThumbs()));
	connect(actSquarish, SIGNAL(triggered()), this, SLOT(setSquarishThumbs()));
	actClassic->setCheckable(true);
	actCompact->setCheckable(true);
	actSquarish->setCheckable(true);
	actClassic->setChecked(GData::thumbsLayout == ThumbView::Classic); 
	actCompact->setChecked(GData::thumbsLayout == ThumbView::Compact); 
	actSquarish->setChecked(GData::thumbsLayout == ThumbView::Squares); 

	refreshAction = new QAction(tr("Reload"), this);
	refreshAction->setIcon(QIcon::fromTheme("view-refresh", QIcon(":/images/refresh.png")));
	connect(refreshAction, SIGNAL(triggered()), this, SLOT(reload()));

	subFoldersAction = new QAction(tr("Include Sub-folders"), this);
	subFoldersAction->setIcon(QIcon(":/images/tree.png"));
	subFoldersAction->setCheckable(true);
	connect(subFoldersAction, SIGNAL(triggered()), this, SLOT(setIncludeSubFolders()));

	pasteAction = new QAction(tr("Paste Here"), this);
	pasteAction->setIcon(QIcon::fromTheme("edit-paste", QIcon(":/images/paste.png")));
	connect(pasteAction, SIGNAL(triggered()), this, SLOT(pasteThumbs()));
	pasteAction->setEnabled(false);
	
	createDirAction = new QAction(tr("New Folder"), this);
	connect(createDirAction, SIGNAL(triggered()), this, SLOT(createSubDirectory()));
	createDirAction->setIcon(QIcon::fromTheme("folder-new", QIcon(":/images/new_folder.png")));
	
	goBackAction = new QAction(tr("Back"), this);
	goBackAction->setIcon(QIcon::fromTheme("go-previous", QIcon(":/images/back.png")));
	connect(goBackAction, SIGNAL(triggered()), this, SLOT(goBack()));
	goBackAction->setEnabled(false);

	goFrwdAction = new QAction(tr("Forward"), this);
	goFrwdAction->setIcon(QIcon::fromTheme("go-next", QIcon(":/images/next.png")));
	connect(goFrwdAction, SIGNAL(triggered()), this, SLOT(goForward()));
	goFrwdAction->setEnabled(false);

	goUpAction = new QAction(tr("Up"), this);
	goUpAction->setIcon(QIcon::fromTheme("go-up", QIcon(":/images/up.png")));
	connect(goUpAction, SIGNAL(triggered()), this, SLOT(goUp()));

	goHomeAction = new QAction(tr("Home"), this);
	connect(goHomeAction, SIGNAL(triggered()), this, SLOT(goHome()));	
	goHomeAction->setIcon(QIcon::fromTheme("go-home", QIcon(":/images/home.png")));

	slideShowAction = new QAction(tr("Slide Show"), this);
	connect(slideShowAction, SIGNAL(triggered()), this, SLOT(slideShow()));
	slideShowAction->setIcon(QIcon::fromTheme("media-playback-start", QIcon(":/images/play.png")));

	nextImageAction = new QAction(tr("Next"), this);
	nextImageAction->setIcon(QIcon::fromTheme("go-next", QIcon(":/images/next.png")));
	connect(nextImageAction, SIGNAL(triggered()), this, SLOT(loadNextImage()));
	
	prevImageAction = new QAction(tr("Previous"), this);
	prevImageAction->setIcon(QIcon::fromTheme("go-previous", QIcon(":/images/back.png")));
	connect(prevImageAction, SIGNAL(triggered()), this, SLOT(loadPrevImage()));

	firstImageAction = new QAction(tr("First"), this);
	firstImageAction->setIcon(QIcon::fromTheme("go-first", QIcon(":/images/first.png")));
	connect(firstImageAction, SIGNAL(triggered()), this, SLOT(loadFirstImage()));

	lastImageAction = new QAction(tr("Last"), this);
	lastImageAction->setIcon(QIcon::fromTheme("go-last", QIcon(":/images/last.png")));
	connect(lastImageAction, SIGNAL(triggered()), this, SLOT(loadLastImage()));

	randomImageAction = new QAction(tr("Random"), this);
	connect(randomImageAction, SIGNAL(triggered()), this, SLOT(loadRandomImage()));

	openAction = new QAction(tr("Open"), this);
	openAction->setIcon(QIcon::fromTheme("document-open", QIcon(":/images/open.png")));
	connect(openAction, SIGNAL(triggered()), this, SLOT(openOp()));

	showClipboardAction = new QAction(tr("Load Clipboard"), this);
	showClipboardAction->setIcon(QIcon::fromTheme("insert-image", QIcon(":/images/new.png")));
	connect(showClipboardAction, SIGNAL(triggered()), this, SLOT(newImage()));

	openWithSubMenu = new QMenu(tr("Open With..."));
	openWithMenuAct = new QAction(tr("Open With..."), this);
	openWithMenuAct->setMenu(openWithSubMenu);
	chooseAppAct = new QAction(tr("Manage External Applications"), this);
	connect(chooseAppAct, SIGNAL(triggered()), this, SLOT(chooseExternalApp()));

	addBookmarkAction = new QAction(tr("Add Bookmark"), this);
	addBookmarkAction->setIcon(QIcon(":/images/new_bookmark.png"));
	connect(addBookmarkAction, SIGNAL(triggered()), this, SLOT(addNewBookmark()));

	removeBookmarkAction = new QAction(tr("Remove Bookmark"), this);
	removeBookmarkAction->setIcon(QIcon::fromTheme("edit-delete", QIcon(":/images/delete.png")));

	zoomOutAct = new QAction(tr("Zoom Out"), this);
	connect(zoomOutAct, SIGNAL(triggered()), this, SLOT(zoomOut()));
	zoomOutAct->setIcon(QIcon::fromTheme("zoom-out", QIcon(":/images/zoom_out.png")));

	zoomInAct = new QAction(tr("Zoom In"), this);
	connect(zoomInAct, SIGNAL(triggered()), this, SLOT(zoomIn()));
	zoomInAct->setIcon(QIcon::fromTheme("zoom-in", QIcon(":/images/zoom_out.png")));

	resetZoomAct = new QAction(tr("Reset Zoom"), this);
	resetZoomAct->setIcon(QIcon::fromTheme("zoom-fit-best", QIcon(":/images/zoom.png")));
	connect(resetZoomAct, SIGNAL(triggered()), this, SLOT(resetZoom()));

	origZoomAct = new QAction(tr("Original Size"), this);
	origZoomAct->setIcon(QIcon::fromTheme("zoom-original", QIcon(":/images/zoom1.png")));
	connect(origZoomAct, SIGNAL(triggered()), this, SLOT(origZoom()));

	keepZoomAct = new QAction(tr("Keep Zoom"), this);
	keepZoomAct->setCheckable(true);
	connect(keepZoomAct, SIGNAL(triggered()), this, SLOT(keepZoom()));

	rotateLeftAct = new QAction(tr("Rotate 90 degree CCW"), this);
	rotateLeftAct->setIcon(QIcon::fromTheme("object-rotate-left", QIcon(":/images/rotate_left.png")));
	connect(rotateLeftAct, SIGNAL(triggered()), this, SLOT(rotateLeft()));

	rotateRightAct = new QAction(tr("Rotate 90 degree CW"), this);
	rotateRightAct->setIcon(QIcon::fromTheme("object-rotate-right", QIcon(":/images/rotate_right.png")));
	connect(rotateRightAct, SIGNAL(triggered()), this, SLOT(rotateRight()));

	flipHAct = new QAction(tr("Flip Horizontally"), this);
	flipHAct->setIcon(QIcon::fromTheme("object-flip-horizontal", QIcon(":/images/flipH.png")));
	connect(flipHAct, SIGNAL(triggered()), this, SLOT(flipHoriz()));

	flipVAct = new QAction(tr("Flip Vertically"), this);
	flipVAct->setIcon(QIcon::fromTheme("object-flip-vertical", QIcon(":/images/flipV.png")));
	connect(flipVAct, SIGNAL(triggered()), this, SLOT(flipVert()));

	cropAct = new QAction(tr("Cropping"), this);
	cropAct->setIcon(QIcon(":/images/crop.png"));
	connect(cropAct, SIGNAL(triggered()), this, SLOT(cropImage()));

	cropToSelectionAct = new QAction(tr("Crop to Selection"), this);
	cropToSelectionAct->setIcon(QIcon(":/images/crop.png"));

	resizeAct = new QAction(tr("Scale Image"), this);
	resizeAct->setIcon(QIcon::fromTheme("transform-scale", QIcon(":/images/scale.png")));
	connect(resizeAct, SIGNAL(triggered()), this, SLOT(scaleImage()));

	freeRotateLeftAct = new QAction(tr("Rotate 1 degree CCW"), this);
	connect(freeRotateLeftAct, SIGNAL(triggered()), this, SLOT(freeRotateLeft()));

	freeRotateRightAct = new QAction(tr("Rotate 1 degree CW"), this);
	connect(freeRotateRightAct, SIGNAL(triggered()), this, SLOT(freeRotateRight()));

	colorsAct = new QAction(tr("Colors"), this);
	connect(colorsAct, SIGNAL(triggered()), this, SLOT(showColorsDialog()));
	colorsAct->setIcon(QIcon(":/images/colors.png"));

	findDupesAction = new QAction(tr("Find Duplicate Images"), this);
	findDupesAction->setIcon(QIcon(":/images/duplicates.png"));
	findDupesAction->setCheckable(true);
	connect(findDupesAction, SIGNAL(triggered()), this, SLOT(findDuplicateImages()));

	mirrorDisabledAct = new QAction(tr("Disable"), this);
	mirrorDualAct = new QAction(tr("Dual"), this);
	mirrorTripleAct = new QAction(tr("Triple"), this);
	mirrorVDualAct = new QAction(tr("Dual Vertical"), this);
	mirrorQuadAct = new QAction(tr("Quad"), this);

	mirrorDisabledAct->setCheckable(true);
	mirrorDualAct->setCheckable(true);
	mirrorTripleAct->setCheckable(true);
	mirrorVDualAct->setCheckable(true);
	mirrorQuadAct->setCheckable(true);
	connect(mirrorDisabledAct, SIGNAL(triggered()), this, SLOT(setMirrorDisabled()));
	connect(mirrorDualAct, SIGNAL(triggered()), this, SLOT(setMirrorDual()));
	connect(mirrorTripleAct, SIGNAL(triggered()), this, SLOT(setMirrorTriple()));
	connect(mirrorVDualAct, SIGNAL(triggered()), this, SLOT(setMirrorVDual()));
	connect(mirrorQuadAct, SIGNAL(triggered()), this, SLOT(setMirrorQuad()));
	mirrorDisabledAct->setChecked(true); 

	keepTransformAct = new QAction(tr("Lock Transformations"), this);
	keepTransformAct->setCheckable(true);
	connect(keepTransformAct, SIGNAL(triggered()), this, SLOT(keepTransformClicked()));

	moveLeftAct = new QAction(tr("Move Left"), this);
	connect(moveLeftAct, SIGNAL(triggered()), this, SLOT(moveLeft()));
	moveRightAct = new QAction(tr("Move Right"), this);
	connect(moveRightAct, SIGNAL(triggered()), this, SLOT(moveRight()));
	moveUpAct = new QAction(tr("Move Up"), this);
	connect(moveUpAct, SIGNAL(triggered()), this, SLOT(moveUp()));
	moveDownAct = new QAction(tr("Move Down"), this);
	connect(moveDownAct, SIGNAL(triggered()), this, SLOT(moveDown()));

	invertSelectionAct = new QAction(tr("Invert Selection"), this);
	connect(invertSelectionAct, SIGNAL(triggered()), thumbView, SLOT(invertSelection()));

	filterImagesFocusAct = new QAction(tr("Filter by Name"), this);
	connect(filterImagesFocusAct, SIGNAL(triggered()), this, SLOT(filterImagesFocus()));
	setPathFocusAct = new QAction(tr("Set Path"), this);
	connect(setPathFocusAct, SIGNAL(triggered()), this, SLOT(setPathFocus()));
}

void Phototonic::createMenus()
{
	fileMenu = menuBar()->addMenu(tr("&File"));
	fileMenu->addAction(subFoldersAction);
	fileMenu->addAction(createDirAction);
	fileMenu->addAction(showClipboardAction);
	fileMenu->addAction(addBookmarkAction);
	fileMenu->addSeparator();
	fileMenu->addAction(exitAction);

	editMenu = menuBar()->addMenu(tr("&Edit"));
	editMenu->addAction(cutAction);
	editMenu->addAction(copyAction);
	editMenu->addAction(copyToAction);
	editMenu->addAction(moveToAction);
	editMenu->addAction(pasteAction);
	editMenu->addAction(renameAction);
	editMenu->addAction(deleteAction);
	editMenu->addSeparator();
	editMenu->addAction(selectAllAction);
	editMenu->addAction(invertSelectionAct);
	addAction(filterImagesFocusAct);
	addAction(setPathFocusAct);
	editMenu->addSeparator();
	editMenu->addAction(settingsAction);

	goMenu = menuBar()->addMenu(tr("&Go"));
	goMenu->addAction(goBackAction);
	goMenu->addAction(goFrwdAction);
	goMenu->addAction(goUpAction);
	goMenu->addAction(goHomeAction);
	goMenu->addSeparator();
	goMenu->addAction(thumbsGoTopAct);
	goMenu->addAction(thumbsGoBottomAct);

	viewMenu = menuBar()->addMenu(tr("&View"));
	viewMenu->addAction(slideShowAction);
	viewMenu->addSeparator();
	
	viewMenu->addAction(thumbsZoomInAct);
	viewMenu->addAction(thumbsZoomOutAct);
	sortMenu = viewMenu->addMenu(tr("Sort By"));
	sortTypesGroup = new QActionGroup(this);
	sortTypesGroup->addAction(actName);
	sortTypesGroup->addAction(actTime);
	sortTypesGroup->addAction(actSize);
	sortTypesGroup->addAction(actType);
	sortMenu->addActions(sortTypesGroup->actions());
	sortMenu->addSeparator();
	sortMenu->addAction(actReverse);
	viewMenu->addSeparator();

	thumbLayoutsGroup = new QActionGroup(this);
	thumbLayoutsGroup->addAction(actClassic);
	thumbLayoutsGroup->addAction(actCompact);
	thumbLayoutsGroup->addAction(actSquarish);
	viewMenu->addActions(thumbLayoutsGroup->actions());
	viewMenu->addSeparator();

	viewMenu->addAction(actShowLabels);
	viewMenu->addAction(actShowHidden);
	viewMenu->addSeparator();
	viewMenu->addAction(refreshAction);

	toolsMenu = menuBar()->addMenu(tr("&Tools"));
	toolsMenu->addAction(findDupesAction);

	menuBar()->addSeparator();
	helpMenu = menuBar()->addMenu(tr("&Help"));
	helpMenu->addAction(aboutAction);

	// thumbview context menu
	thumbView->addAction(openAction);
	thumbView->addAction(openWithMenuAct);
	thumbView->addAction(cutAction);
	thumbView->addAction(copyAction);
	thumbView->addAction(pasteAction);
	addMenuSeparator(thumbView);
	thumbView->addAction(copyToAction);
	thumbView->addAction(moveToAction);
	thumbView->addAction(renameAction);
	thumbView->addAction(deleteAction);
	addMenuSeparator(thumbView);
	thumbView->addAction(selectAllAction);
	thumbView->addAction(invertSelectionAct);
	thumbView->setContextMenuPolicy(Qt::ActionsContextMenu);
	menuBar()->setVisible(true);
}

void Phototonic::createToolBars()
{
	/* Edit */
	editToolBar = addToolBar(tr("Edit"));
	editToolBar->setObjectName("Edit");
	editToolBar->addAction(cutAction);
	editToolBar->addAction(copyAction);
	editToolBar->addAction(pasteAction);
	editToolBar->addAction(deleteAction);
	editToolBar->addAction(showClipboardAction);
	connect(editToolBar->toggleViewAction(), SIGNAL(triggered()), this, SLOT(setEditToolBarVisibility()));

	/* Navigation */
	goToolBar = addToolBar(tr("Navigation"));
	goToolBar->setObjectName("Navigation");
	goToolBar->addAction(goBackAction);
	goToolBar->addAction(goFrwdAction);
	goToolBar->addAction(goUpAction);
	goToolBar->addAction(goHomeAction);
	goToolBar->addAction(refreshAction);

	/* path bar */
	pathBar = new QLineEdit;
	pathComplete = new QCompleter(this);
	QDirModel *pathCompleteDirMod = new QDirModel;
	pathCompleteDirMod->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
	pathComplete->setModel(pathCompleteDirMod);
	pathBar->setCompleter(pathComplete);
	pathBar->setMinimumWidth(200);
	pathBar->setMaximumWidth(300);
	connect(pathBar, SIGNAL(returnPressed()), this, SLOT(goPathBarDir()));
	goToolBar->addWidget(pathBar);
	goToolBar->addAction(subFoldersAction);
	goToolBar->addAction(findDupesAction);
	connect(goToolBar->toggleViewAction(), SIGNAL(triggered()), this, SLOT(setGoToolBarVisibility()));

	/* View */
	viewToolBar = addToolBar(tr("View"));
	viewToolBar->setObjectName("View");
	viewToolBar->addAction(slideShowAction);
	viewToolBar->addAction(thumbsZoomInAct);
	viewToolBar->addAction(thumbsZoomOutAct);

	/* filter bar */ 
	QAction *filterAct = new QAction(tr("Filter"), this);
	filterAct->setIcon(QIcon::fromTheme("edit-find", QIcon(":/images/zoom.png")));
	connect(filterAct, SIGNAL(triggered()), this, SLOT(setThumbsFilter()));
	filterBar = new QLineEdit;
	filterBar->setMinimumWidth(100);
	filterBar->setMaximumWidth(200);
	connect(filterBar, SIGNAL(returnPressed()), this, SLOT(setThumbsFilter()));
	connect(filterBar, SIGNAL(textChanged(const QString&)), this, SLOT(clearThumbsFilter()));
	filterBar->setClearButtonEnabled(true);
	filterBar->addAction(filterAct, QLineEdit::LeadingPosition);

	viewToolBar->addSeparator();
	viewToolBar->addWidget(filterBar);
	viewToolBar->addAction(settingsAction);
	connect(viewToolBar->toggleViewAction(), SIGNAL(triggered()), this, SLOT(setViewToolBarVisibility()));	

	/* image */
	imageToolBar = addToolBar(tr("Image"));
	imageToolBar->setObjectName("Image");
	imageToolBar->addAction(prevImageAction);
	imageToolBar->addAction(nextImageAction);
	imageToolBar->addAction(firstImageAction);
	imageToolBar->addAction(lastImageAction);
	imageToolBar->addAction(slideShowAction);
	imageToolBar->addSeparator();
	imageToolBar->addAction(saveAction);
	imageToolBar->addAction(saveAsAction);
	imageToolBar->addAction(deleteAction);
	imageToolBar->addSeparator();
	imageToolBar->addAction(zoomInAct);
	imageToolBar->addAction(zoomOutAct);
	imageToolBar->addAction(resetZoomAct);
	imageToolBar->addAction(origZoomAct);
	imageToolBar->addSeparator();
	imageToolBar->addAction(resizeAct);
	imageToolBar->addAction(rotateRightAct);
	imageToolBar->addAction(rotateLeftAct);
	imageToolBar->addAction(flipHAct);
	imageToolBar->addAction(flipVAct);
	imageToolBar->addAction(cropAct);
	imageToolBar->addAction(colorsAct);
	imageToolBar->setVisible(false);
	connect(imageToolBar->toggleViewAction(), SIGNAL(triggered()), this, SLOT(setImageToolBarVisibility()));

	setToolbarIconSize();
}

void Phototonic::setToolbarIconSize()
{
	int iconSize;
	if (initComplete)
		GData::smallIcons = actSmallIcons->isChecked();
	iconSize = GData::smallIcons? 16 : 24;
	QSize iconQSize(iconSize, iconSize);

	editToolBar->setIconSize(iconQSize);
	goToolBar->setIconSize(iconQSize);
	viewToolBar->setIconSize(iconQSize);
	imageToolBar->setIconSize(iconQSize);
}

void Phototonic::createStatusBar()
{
	stateLabel = new QLabel(tr("Initializing..."));
	statusBar()->addWidget(stateLabel);

	busyMovie = new QMovie(":/images/busy.gif");
	busyLabel = new QLabel(this);
	busyLabel->setMovie(busyMovie);
	statusBar()->addWidget(busyLabel);
	busyLabel->setVisible(false);

	statusBar()->setStyleSheet("QStatusBar::item { border: 0px solid black }; ");
}

void Phototonic::createFSTree()
{
	fsDock = new QDockWidget(tr("File System"), this);
	fsDock->setObjectName("File System");

	fsTree = new FSTree(fsDock);
	fsDock->setWidget(fsTree);
	connect(fsDock->toggleViewAction(), SIGNAL(triggered()), this, SLOT(setFsDockVisibility()));	
	connect(fsDock, SIGNAL(visibilityChanged(bool)), this, SLOT(setFsDockVisibility()));	
	addDockWidget(Qt::LeftDockWidgetArea, fsDock);

	// Context menu
	fsTree->addAction(openAction);
	fsTree->addAction(createDirAction);
	fsTree->addAction(renameAction);
	fsTree->addAction(deleteAction);
	addMenuSeparator(fsTree);
	fsTree->addAction(pasteAction);
	addMenuSeparator(fsTree);
	fsTree->addAction(openWithMenuAct);
	fsTree->addAction(addBookmarkAction);
	fsTree->setContextMenuPolicy(Qt::ActionsContextMenu);

	connect(fsTree, SIGNAL(clicked(const QModelIndex&)),
				this, SLOT(goSelectedDir(const QModelIndex &)));

	connect(fsTree->fsModel, SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
				this, SLOT(checkDirState(const QModelIndex &, int, int)));

	connect(fsTree, SIGNAL(dropOp(Qt::KeyboardModifiers, bool, QString)),
				this, SLOT(dropOp(Qt::KeyboardModifiers, bool, QString)));

	fsTree->setCurrentIndex(fsTree->fsModel->index(QDir::currentPath()));
}

void Phototonic::createBookmarks()
{
	bmDock = new QDockWidget(tr("Bookmarks"), this);
	bmDock->setObjectName("Bookmarks");
	bookmarks = new BookMarks(bmDock);
	bmDock->setWidget(bookmarks);
	
	connect(bmDock->toggleViewAction(), SIGNAL(triggered()), this, SLOT(setBmDockVisibility()));	
	connect(bmDock, SIGNAL(visibilityChanged(bool)), this, SLOT(setBmDockVisibility()));	
	connect(bookmarks, SIGNAL(itemClicked(QTreeWidgetItem *, int)),
					this, SLOT(bookmarkClicked(QTreeWidgetItem *, int)));
	connect(removeBookmarkAction, SIGNAL(triggered()), bookmarks, SLOT(removeBookmark()));
	connect(bookmarks, SIGNAL(dropOp(Qt::KeyboardModifiers, bool, QString)),
				this, SLOT(dropOp(Qt::KeyboardModifiers, bool, QString)));
	addDockWidget(Qt::LeftDockWidgetArea, bmDock);

	bookmarks->addAction(pasteAction);
	bookmarks->addAction(removeBookmarkAction);
	bookmarks->setContextMenuPolicy(Qt::ActionsContextMenu);
}

void Phototonic::sortThumbnains()
{
	thumbView->thumbsSortFlags = QDir::IgnoreCase;

	if (actName->isChecked())
		thumbView->thumbsSortFlags |= QDir::Name;
	else if (actTime->isChecked())
		thumbView->thumbsSortFlags |= QDir::Time;
	else if (actSize->isChecked())
		thumbView->thumbsSortFlags |= QDir::Size;
	else if (actType->isChecked())
		thumbView->thumbsSortFlags |= QDir::Type;

	if (actReverse->isChecked())
		thumbView->thumbsSortFlags |= QDir::Reversed;

	refreshThumbs(false);
}

void Phototonic::reload()
{
	findDupesAction->setChecked(false);
	if (GData::layoutMode == thumbViewIdx) {
		refreshThumbs(false);
	} else {
		imageView->reload();
	}
}

void Phototonic::setIncludeSubFolders()
{
	findDupesAction->setChecked(false);
	GData::includeSubFolders = subFoldersAction->isChecked();
	refreshThumbs(false);
}

void Phototonic::refreshThumbs(bool scrollToTop)
{
	thumbView->setNeedScroll(scrollToTop);
	QTimer::singleShot(0, this, SLOT(reloadThumbsSlot()));
	QTimer::singleShot(100, this, SLOT(selectRecentThumb()));
}

void Phototonic::setClassicThumbs()
{
	GData::thumbsLayout = ThumbView::Classic;
	actShowLabels->setEnabled(true);
	refreshThumbs(false);
}

void Phototonic::setCompactThumbs()
{
	GData::thumbsLayout = ThumbView::Compact;
	actShowLabels->setEnabled(true);
	refreshThumbs(false);
}

void Phototonic::setSquarishThumbs()
{
	GData::thumbsLayout = ThumbView::Squares;
	actShowLabels->setEnabled(false);
	refreshThumbs(false);
}

void Phototonic::showHiddenFiles()
{
	GData::showHiddenFiles = actShowHidden->isChecked();
	fsTree->setModelFlags();
	refreshThumbs(false);
}

void Phototonic::toggleImageToolbar()
{
	imageToolBar->setVisible(actShowViewerToolbars->isChecked());
	GData::imageToolbarFullScreen = actShowViewerToolbars->isChecked();
}

void Phototonic::showLabels()
{
	GData::showLabels = actShowLabels->isChecked();
	refreshThumbs(false);
}

void Phototonic::about()
{
	QString aboutString = "<h2>Phototonic v1.5.32</h2>"
		+ tr("<p>Image viewer and organizer</p>")
		+ "Qt v" + QT_VERSION_STR
		+ "<p><a href=\"http://oferkv.github.io/phototonic/\">" + tr("Home page") + "</a></p>"
		+ "<p><a href=\"https://github.com/oferkv/phototonic/issues\">" + tr("Bug reports") + "</a></p>"
		+ "<p>Copyright &copy; 2013-2014 Ofer Kashayov (oferkv@live.com)</p>"
		+ tr("Contributors / Code:") + "<br>"
		+ "Christopher Roy Bratusek (nano@jpberlin.de)<br>"
		+ "Krzysztof Pyrkosz (pyrkosz@o2.pl)<br>"
		+ "<br>" + tr("Contributors / Translations:")
		+ "<table><tr><td>Czech:</td><td>Pavel Fric (pavelfric@seznam.cz)</td></tr>"
		+ "<tr><td>French:</td><td>David Geiger (david.david@mageialinux-online.org)</td></tr>"
		+ "<tr><td></td><td>Adrien Daugabel (adrien.d@mageialinux-online.org)</td></tr>"
		+ "<tr><td></td><td>Rémi Verschelde (akien@mageia.org)</td></tr>"
		+ "<tr><td>German:</td><td>Jonathan Hooverman (jonathan.hooverman@gmail.com)</td></tr>"
		+ QString::fromUtf8("<tr><td>Polish:</td><td>Robert Wojew\u00F3dzki (robwoj44@poczta.onet.pl)</td></tr>")
		+ "<tr><td>Russian:</td><td>Ilya Alexandrovich (yast4ik@gmail.com)</td></tr></table>"
		+ "<p>Phototonic is licensed under the GNU General Public License version 3</p>";

	QMessageBox::about(this, tr("About") + " Phototonic", aboutString);
}

void Phototonic::filterImagesFocus()
{
	if (GData::layoutMode == thumbViewIdx)
	{
		if (!viewToolBar->isVisible())
			viewToolBar->setVisible(true);
		setViewToolBarVisibility();
		filterBar->setFocus(Qt::OtherFocusReason);
		filterBar->selectAll();
	}
}

void Phototonic::setPathFocus()
{
	if (GData::layoutMode == thumbViewIdx)
	{
		if (!goToolBar->isVisible())
			goToolBar->setVisible(true);
		setGoToolBarVisibility();
		pathBar->setFocus(Qt::OtherFocusReason);
		pathBar->selectAll();
	}
}

void Phototonic::cleanupSender()
{
	delete QObject::sender();
}

void Phototonic::externalAppError()
{
	QMessageBox msgBox;
	msgBox.critical(this, tr("Error"), tr("Failed to start external application."));
}

void Phototonic::runExternalApp()
{
	QString execCommand;
	QString selectedFileNames("");
	execCommand = GData::externalApps[((QAction*) sender())->text()];

	if (GData::layoutMode == imageViewIdx) {
		if (imageView->isNewImage()) 	{
			showNewImageWarning(this);
			return;
		}

		execCommand += " \"" + imageView->currentImageFullPath + "\"";
	} else {
		if (QApplication::focusWidget() == fsTree) {
			selectedFileNames += " \"" + getSelectedPath() + "\"";
		} else {

			QModelIndexList selectedIdxList = thumbView->selectionModel()->selectedIndexes();
			if (selectedIdxList.size() < 1)
			{
				setStatus(tr("Invalid selection."));
				return;
			}

			selectedFileNames += " ";
			for (int tn = selectedIdxList.size() - 1; tn >= 0 ; --tn)
			{
				selectedFileNames += "\"" +
					thumbView->thumbViewModel->item(selectedIdxList[tn].row())->data(thumbView->FileNameRole).toString();
				if (tn) 
					selectedFileNames += "\" ";
			}
		}
		
		execCommand += selectedFileNames;
	}

	QProcess *externalProcess = new QProcess();
	connect(externalProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(cleanupSender()));
	connect(externalProcess, SIGNAL(error(QProcess::ProcessError)), this, SLOT(externalAppError()));
	externalProcess->start(execCommand);
}

void Phototonic::updateExternalApps()
{
	int actionNum = 0;
	QMapIterator<QString, QString> eaIter(GData::externalApps);

    QList<QAction*> actionList = openWithSubMenu->actions();
    if (!actionList.empty()) {

    	for (int i = 0; i < actionList.size(); ++i)
    	{
      		QAction *action = actionList.at(i);
      		if (action->isSeparator())
      			break;
			openWithSubMenu->removeAction(action);
			imageView->removeAction(action);
			delete action;
   		}

      	openWithSubMenu->clear();
    }

	while (eaIter.hasNext())
	{
		++actionNum;
		eaIter.next();
		QAction *extAppAct = new QAction(eaIter.key(), this);
		if (actionNum < 10)
			extAppAct->setShortcut(QKeySequence("Alt+" + QString::number(actionNum)));
		extAppAct->setIcon(QIcon::fromTheme(eaIter.key()));
		connect(extAppAct, SIGNAL(triggered()), this, SLOT(runExternalApp()));
		openWithSubMenu->addAction(extAppAct);
		imageView->addAction(extAppAct);
	}

	openWithSubMenu->addSeparator();
	openWithSubMenu->addAction(chooseAppAct);
}

void Phototonic::chooseExternalApp()
{
	AppMgmtDialog *dialog = new AppMgmtDialog(this);
	dialog->exec();
	updateExternalApps();
	delete(dialog);
}

void Phototonic::showSettings()
{
	if (GData::slideShowActive)
		slideShow();
	imageView->setCursorHiding(false);
	
	SettingsDialog *dialog = new SettingsDialog(this);
	if (dialog->exec())
	{
		imageView->setPalette(QPalette(GData::backgroundColor));
		thumbView->setThumbColors();
		GData::imageZoomFactor = 1.0;
		imageView->infoLabel->setVisible(GData::enableImageInfoFS && isFullScreen());

		if (GData::layoutMode == imageViewIdx)
		{
			imageView->reload();
			needThumbsRefresh = true;
		}
		else
			refreshThumbs(false);
	}

	if (isFullScreen())
		imageView->setCursorHiding(true);
	delete dialog;
}

void Phototonic::toggleFullScreen()
{
	if (fullScreenAct->isChecked())
	{
		shouldMaximize = isMaximized();
		showFullScreen();
		GData::isFullScreen = true;
		imageView->setCursorHiding(true);
		imageView->infoLabel->setVisible(GData::enableImageInfoFS);
	}
	else
	{
		showNormal();
		if (shouldMaximize)
			showMaximized();
		imageView->setCursorHiding(false);
		GData::isFullScreen = false;
		imageView->infoLabel->setVisible(false);
	}
}

void Phototonic::selectAllThumbs()
{
	thumbView->selectAll();
}

void Phototonic::copyOrCutThumbs(bool copy)
{
	GData::copyCutIdxList = thumbView->selectionModel()->selectedIndexes();
	copyCutCount = GData::copyCutIdxList.size();

	GData::copyCutFileList.clear();
	for (int tn = 0; tn < copyCutCount; ++tn)
	{
		GData::copyCutFileList.append(thumbView->thumbViewModel->item(GData::copyCutIdxList[tn].
										row())->data(thumbView->FileNameRole).toString());
	}

	GData::copyOp = copy;
	pasteAction->setEnabled(true);
}

void Phototonic::cutThumbs()
{
	copyOrCutThumbs(false);
}

void Phototonic::copyThumbs()
{
	copyOrCutThumbs(true);
}

void Phototonic::copyImagesTo()
{
	copyMoveImages(false);
}

void Phototonic::moveImagesTo()
{
	copyMoveImages(true);
}

void Phototonic::copyMoveImages(bool move)
{
	copyMoveToDialog = new CopyMoveToDialog(this, getSelectedPath(), move);
	if (copyMoveToDialog->exec()) {
		if (GData::layoutMode == thumbViewIdx) {
			if (copyMoveToDialog->copyOp)
				copyThumbs();
			else
				cutThumbs();

			pasteThumbs();
		} else {
			if (imageView->isNewImage()) 	{
				showNewImageWarning(this);
				return;
			}
		
			QFileInfo fileInfo = QFileInfo(imageView->currentImageFullPath);
			QString fileName = fileInfo.fileName();
			QString destFile = copyMoveToDialog->selectedPath + QDir::separator() + fileInfo.fileName();
			
			int res = cpMvFile(copyMoveToDialog->copyOp, fileName, imageView->currentImageFullPath,
				 									destFile, copyMoveToDialog->selectedPath);

			if (!res) {
				QMessageBox msgBox;
				msgBox.critical(this, tr("Error"), tr("Failed to copy or move image."));
			} else {
				if (!copyMoveToDialog->copyOp) {
					int currentRow = thumbView->getCurrentRow();
					thumbView->thumbViewModel->removeRow(currentRow);
					updateCurrentImage(currentRow);
				}
			}
		}
	}

	bookmarks->reloadBookmarks();
	delete(copyMoveToDialog);
	copyMoveToDialog = 0;
}

void Phototonic::thumbsZoomIn()
{
	if (thumbView->thumbSize < THUMB_SIZE_MAX)
	{
		thumbView->thumbSize += 50;
		thumbsZoomOutAct->setEnabled(true);
		if (thumbView->thumbSize == THUMB_SIZE_MAX)
			thumbsZoomInAct->setEnabled(false);
		refreshThumbs(false);
	}
}

void Phototonic::thumbsZoomOut()
{
	if (thumbView->thumbSize > THUMB_SIZE_MIN)
	{
		thumbView->thumbSize -= 50;
		thumbsZoomInAct->setEnabled(true);
		if (thumbView->thumbSize == THUMB_SIZE_MIN)
			thumbsZoomOutAct->setEnabled(false);
		refreshThumbs(false);
	}
}

void Phototonic::zoomOut()
{
	GData::imageZoomFactor -= (GData::imageZoomFactor <= 0.25)? 0 : 0.25;
	imageView->tempDisableResize = false;
	imageView->resizeImage();
	imageView->setFeedback(tr("Zoom %1%").arg(QString::number(GData::imageZoomFactor * 100)));
}

void Phototonic::zoomIn()
{
	GData::imageZoomFactor += (GData::imageZoomFactor >= 3.50)? 0 : 0.25;
	imageView->tempDisableResize = false;
	imageView->resizeImage();
	imageView->setFeedback(tr("Zoom %1%").arg(QString::number(GData::imageZoomFactor * 100)));
}

void Phototonic::resetZoom()
{
	GData::imageZoomFactor = 1.0;
	imageView->tempDisableResize = false;
	imageView->resizeImage();
	imageView->setFeedback(tr("Zoom Reset"));
}

void Phototonic::origZoom()
{
	GData::imageZoomFactor = 1.0;
	imageView->tempDisableResize = true;
	imageView->resizeImage();
	imageView->setFeedback(tr("Original Size"));
}

void Phototonic::keepZoom()
{
	GData::keepZoomFactor = keepZoomAct->isChecked();
	if (GData::keepZoomFactor)
		imageView->setFeedback(tr("Zoom Locked"));
	else
		imageView->setFeedback(tr("Zoom Unlocked"));
}

void Phototonic::keepTransformClicked()
{
	GData::keepTransform = keepTransformAct->isChecked();

	if (GData::keepTransform) {
		imageView->setFeedback(tr("Transformations Locked"));
		if (cropDialog)
			cropDialog->applyCrop(0);
	} else {
		GData::cropLeftPercent = GData::cropTopPercent = GData::cropWidthPercent = GData::cropHeightPercent = 0;
		imageView->setFeedback(tr("Transformations Unlocked"));
	}

	imageView->refresh();
}

void Phototonic::rotateLeft()
{
	GData::rotation -= 90;
	if (GData::rotation < 0)
		GData::rotation = 270;
	imageView->refresh();
	imageView->setFeedback(tr("Rotation %1°").arg(QString::number(GData::rotation)));
}

void Phototonic::rotateRight()
{
	GData::rotation += 90;
	if (GData::rotation > 270)
		GData::rotation = 0;
	imageView->refresh();
	imageView->setFeedback(tr("Rotation %1°").arg(QString::number(GData::rotation)));
}

void Phototonic::flipVert()
{
	GData::flipV = !GData::flipV;
	imageView->refresh();
	imageView->setFeedback(GData::flipV? tr("Flipped Vertically") : tr("Unflipped Vertically"));
}

void Phototonic::flipHoriz()
{
	GData::flipH = !GData::flipH;
	imageView->refresh();
	imageView->setFeedback(GData::flipH? tr("Flipped Horizontally") : tr("Unflipped Horizontally"));
}

void Phototonic::cropImage()
{
	if (GData::slideShowActive)
		slideShow();

	if (!cropDialog) {
		cropDialog = new CropDialog(this, imageView);
		connect(cropDialog, SIGNAL(accepted()), this, SLOT(cleanupCropDialog()));
		connect(cropDialog, SIGNAL(rejected()), this, SLOT(cleanupCropDialog()));
	}

	cropDialog->show();
	setInterfaceEnabled(false);
	cropDialog->applyCrop(0);
}

void Phototonic::scaleImage()
{
	if (GData::slideShowActive)
		slideShow();

	if (GData::layoutMode == thumbViewIdx && thumbView->selectionModel()->selectedIndexes().size() < 1) {
		setStatus(tr("No selection"));
		return;
	}

	resizeDialog = new ResizeDialog(this, imageView);
	connect(resizeDialog, SIGNAL(accepted()), this, SLOT(cleanupScaleDialog()));
	connect(resizeDialog, SIGNAL(rejected()), this, SLOT(cleanupScaleDialog()));

	resizeDialog->show();
	setInterfaceEnabled(false);
}

void Phototonic::freeRotateLeft()
{
	--GData::rotation;
	if (GData::rotation < 0)
		GData::rotation = 359;
	imageView->refresh();
	imageView->setFeedback(tr("Rotation %1°").arg(QString::number(GData::rotation)));
}

void Phototonic::freeRotateRight()
{
	++GData::rotation;
	if (GData::rotation > 360)
		GData::rotation = 1;
	imageView->refresh();
	imageView->setFeedback(tr("Rotation %1°").arg(QString::number(GData::rotation)));
}

void Phototonic::showColorsDialog()
{
	if (GData::slideShowActive)
		slideShow();

	if (!colorsDialog) {
		colorsDialog = new ColorsDialog(this, imageView);
		connect(colorsDialog, SIGNAL(accepted()), this, SLOT(cleanupColorsDialog()));
		connect(colorsDialog, SIGNAL(rejected()), this, SLOT(cleanupColorsDialog()));
	}

	GData::colorsActive = true;
	colorsDialog->show();
	colorsDialog->applyColors(0);
	setInterfaceEnabled(false);
}

void Phototonic::moveRight()
{
	imageView->keyMoveEvent(ImageView::MoveRight);
}

void Phototonic::moveLeft()
{
	imageView->keyMoveEvent(ImageView::MoveLeft);
}

void Phototonic::moveUp()
{
	imageView->keyMoveEvent(ImageView::MoveUp);
}

void Phototonic::moveDown()
{
	imageView->keyMoveEvent(ImageView::MoveDown);
}

void Phototonic::setMirrorDisabled()
{
	imageView->mirrorLayout = ImageView::LayNone;
	imageView->refresh();
	imageView->setFeedback(tr("Mirroring Disabled"));
}

void Phototonic::setMirrorDual()
{
	imageView->mirrorLayout = ImageView::LayDual;
	imageView->refresh();
	imageView->setFeedback(tr("Mirroring: Dual"));
}

void Phototonic::setMirrorTriple()
{
	imageView->mirrorLayout = ImageView::LayTriple;
	imageView->refresh();
	imageView->setFeedback(tr("Mirroring: Triple"));
}

void Phototonic::setMirrorVDual()
{
	imageView->mirrorLayout = ImageView::LayVDual;
	imageView->refresh();
	imageView->setFeedback(tr("Mirroring: Dual Vertical"));
}

void Phototonic::setMirrorQuad()
{
	imageView->mirrorLayout = ImageView::LayQuad;
	imageView->refresh();
	imageView->setFeedback(tr("Mirroring: Quad"));
}

bool Phototonic::isValidPath(QString &path)
{
	QDir checkPath(path);
	if (!checkPath.exists() || !checkPath.isReadable()) {
		return false;
	}
	return true;
}

void Phototonic::pasteThumbs()
{
	if (!copyCutCount)
		return;

	QString destDir;
	if (copyMoveToDialog)
		destDir = copyMoveToDialog->selectedPath;
	else {
		if (QApplication::focusWidget() == bookmarks) {
			if (bookmarks->currentItem()) {
				destDir = bookmarks->currentItem()->toolTip(0);
			}
		} else {
			destDir = getSelectedPath();
		}
	}
	
	if (!isValidPath(destDir)) {
		QMessageBox msgBox;
		msgBox.critical(this, tr("Error"), tr("Can not copy or move to ") + destDir);
		selectCurrentViewDir();
		return;
	}

	bool pasteInCurrDir = (thumbView->currentViewDir == destDir);

	QFileInfo fileInfo;
	if (!GData::copyOp && pasteInCurrDir) {
		for (int tn = 0; tn < GData::copyCutFileList.size(); ++tn) {
			fileInfo = QFileInfo(GData::copyCutFileList[tn]);
			if (fileInfo.absolutePath() == destDir) {
				QMessageBox msgBox;
				msgBox.critical(this, tr("Error"), tr("Can not copy or move to the same folder"));
				return;
			}
		}
	}

	CpMvDialog *dialog = new CpMvDialog(this);
	dialog->exec(thumbView, destDir, pasteInCurrDir);
	if (pasteInCurrDir) {
		for (int tn = 0; tn < GData::copyCutFileList.size(); ++tn) {
			thumbView->addThumb(GData::copyCutFileList[tn]);
		}
	}

	QString state = QString((GData::copyOp? tr("Copied") : tr("Moved")) + " " +
								tr("%n image(s)", "", dialog->nfiles));
	setStatus(state);
	delete(dialog);
	selectCurrentViewDir();

	copyCutCount = 0;
	GData::copyCutIdxList.clear();
	GData::copyCutFileList.clear();
	pasteAction->setEnabled(false);

	thumbView->loadVisibleThumbs();
}

void Phototonic::updateCurrentImage(int currentRow)
{
	bool wrapImageListTmp = GData::wrapImageList;
	GData::wrapImageList = false;

	if (currentRow == thumbView->thumbViewModel->rowCount())
	{
		thumbView->setCurrentRow(currentRow - 1);
	}

	if (thumbView->getNextRow() < 0 && currentRow > 0)
	{
		imageView->loadImage(thumbView->thumbViewModel->item(currentRow - 1)->
											data(thumbView->FileNameRole).toString());
	}
	else
	{
		if (thumbView->thumbViewModel->rowCount() == 0)
		{
			hideViewer();
			refreshThumbs(true);
			return;
		}
		imageView->loadImage(thumbView->thumbViewModel->item(currentRow)->
											data(thumbView->FileNameRole).toString());
	}
		
	GData::wrapImageList = wrapImageListTmp;
	thumbView->setImageviewWindowTitle();
}

void Phototonic::deleteViewerImage()
{
	if (imageView->isNewImage())
	{
		showNewImageWarning(this);
		return;
	}

	bool ok;
	QFileInfo fileInfo = QFileInfo(imageView->currentImageFullPath);
	QString fileName = fileInfo.fileName();

	QMessageBox msgBox;
	msgBox.setText(tr("Permanently delete") + " " + fileName);
	msgBox.setWindowTitle(tr("Delete image"));
	msgBox.setIcon(QMessageBox::Warning);
	msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
	msgBox.setDefaultButton(QMessageBox::Cancel);
	msgBox.setButtonText(QMessageBox::Yes, tr("Yes"));  
    msgBox.setButtonText(QMessageBox::Cancel, tr("Cancel"));  
	int ret = msgBox.exec();

	if (ret == QMessageBox::Yes)
	{
		int currentRow = thumbView->getCurrentRow();

		ok = QFile::remove(imageView->currentImageFullPath);
		if (ok)
		{
			 thumbView->thumbViewModel->removeRow(currentRow);
		}
		else
		{
			QMessageBox msgBox;
			msgBox.critical(this, tr("Error"), tr("Failed to delete image"));
			return;
		}

		updateCurrentImage(currentRow);
	}
}

void Phototonic::deleteOp()
{
	if (QApplication::focusWidget() == fsTree)
	{
		deleteDir();
		return;
	}

	if (GData::layoutMode == imageViewIdx)
	{
		deleteViewerImage();
		return;
	}

	if (thumbView->selectionModel()->selectedIndexes().size() < 1)
	{
		setStatus(tr("No selection"));
		return;
	}

	QMessageBox msgBox;
	msgBox.setText(tr("Permanently delete selected images?"));
	msgBox.setWindowTitle(tr("Delete images"));
	msgBox.setIcon(QMessageBox::Warning);
	msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
	msgBox.setDefaultButton(QMessageBox::Cancel);
	msgBox.setButtonText(QMessageBox::Yes, tr("Yes"));  
    msgBox.setButtonText(QMessageBox::Cancel, tr("Cancel"));  
	int ret = msgBox.exec();

	if (ret == QMessageBox::Yes)
	{
		QModelIndexList indexesList;
		int nfiles = 0;
		bool ok;
	
		while((indexesList = thumbView->selectionModel()->selectedIndexes()).size())
		{
			ok = QFile::remove(thumbView->thumbViewModel->item(
								indexesList.first().row())->data(thumbView->FileNameRole).toString());

			++nfiles;
			if (ok)
			{
				 thumbView->thumbViewModel->removeRow(indexesList.first().row());
			}
			else
			{
				QMessageBox msgBox;
				msgBox.critical(this, tr("Error"), tr("Failed to delete image."));
				return;
			}
		}
		
		QString state = QString(tr("Deleted") + " " + tr("%n image(s)", "", nfiles));
		setStatus(state);

		thumbView->loadVisibleThumbs();
	}
}

void Phototonic::goTo(QString path)
{
	findDupesAction->setChecked(false);
	thumbView->setNeedScroll(true);
	fsTree->setCurrentIndex(fsTree->fsModel->index(path));
	thumbView->currentViewDir = path;
	refreshThumbs(true);
}

void Phototonic::goSelectedDir(const QModelIndex &idx)
{
	findDupesAction->setChecked(false);
	thumbView->setNeedScroll(true);
	thumbView->currentViewDir = getSelectedPath();
	refreshThumbs(true);
	fsTree->expand(idx);
}

void Phototonic::goPathBarDir()
{
	findDupesAction->setChecked(false);
	thumbView->setNeedScroll(true);

	QDir checkPath(pathBar->text());
	if (!checkPath.exists() || !checkPath.isReadable())
	{
		QMessageBox msgBox;
		msgBox.critical(this, tr("Error"), tr("Invalid Path:") + " " + pathBar->text());
		pathBar->setText(thumbView->currentViewDir);
		return;
	}
	
	thumbView->currentViewDir = pathBar->text();
	refreshThumbs(true);
	selectCurrentViewDir();
}

void Phototonic::bookmarkClicked(QTreeWidgetItem *item, int col)
{
	goTo(item->toolTip(col));
}

void Phototonic::setThumbsFilter()
{
	thumbView->filterStr = filterBar->text();
	refreshThumbs(true);
}

void Phototonic::clearThumbsFilter()
{
	if (filterBar->text() == "")
	{
		thumbView->filterStr = filterBar->text();
		refreshThumbs(true);
	}
}

void Phototonic::goBack()
{
	if (currentHistoryIdx > 0)
	{
		needHistoryRecord = false;
		goTo(pathHistory.at(--currentHistoryIdx));
		goFrwdAction->setEnabled(true);
		if (currentHistoryIdx == 0)
			goBackAction->setEnabled(false);
	}
}

void Phototonic::goForward()
{

	if (currentHistoryIdx < pathHistory.size() - 1)
	{
		needHistoryRecord = false;
		goTo(pathHistory.at(++currentHistoryIdx));
		if (currentHistoryIdx == (pathHistory.size() - 1))
			goFrwdAction->setEnabled(false);
	}
}

void Phototonic::goUp()
{
	QFileInfo fileInfo = QFileInfo(thumbView->currentViewDir);
	goTo(fileInfo.dir().absolutePath());
}

void Phototonic::goHome()
{
	goTo(QDir::homePath());
}

void Phototonic::setCopyCutActions(bool setEnabled)
{
	cutAction->setEnabled(setEnabled);
	copyAction->setEnabled(setEnabled);
}

void Phototonic::changeActionsBySelection(const QItemSelection&, const QItemSelection&)
{
	setCopyCutActions(thumbView->selectionModel()->selectedIndexes().size());
}

void Phototonic::updateActions(QWidget*, QWidget *selectedWidget)
{
	if (selectedWidget == thumbView) {
		setCopyCutActions(thumbView->selectionModel()->selectedIndexes().size());
	} else {
		setCopyCutActions(false);
	}

	if (GData::layoutMode == imageViewIdx && !interfaceDisabled) {
		setViewerKeyEventsEnabled(true);
		fullScreenAct->setEnabled(true);
		closeImageAct->setEnabled(true);

	} else {
		if (selectedWidget == imageView->scrlArea) {
			setViewerKeyEventsEnabled(true);
			fullScreenAct->setEnabled(false);
			closeImageAct->setEnabled(false);
		} else {
			setViewerKeyEventsEnabled(false);
			fullScreenAct->setEnabled(false);
			closeImageAct->setEnabled(false);
		}

	}
}

void Phototonic::writeSettings()
{
	if (GData::layoutMode == thumbViewIdx) {
		GData::appSettings->setValue("Geometry", saveGeometry());
		GData::appSettings->setValue("WindowState", saveState());
	}

	GData::appSettings->setValue("thumbsSortFlags", (int)thumbView->thumbsSortFlags);
	GData::appSettings->setValue("thumbsZoomVal", (int)thumbView->thumbSize);
	GData::appSettings->setValue("isFullScreen", (bool)GData::isFullScreen);
	GData::appSettings->setValue("backgroundColor", GData::backgroundColor);
	GData::appSettings->setValue("backgroundThumbColor", GData::thumbsBackgroundColor);
	GData::appSettings->setValue("textThumbColor", GData::thumbsTextColor);
	GData::appSettings->setValue("thumbSpacing", (int)GData::thumbSpacing);
	GData::appSettings->setValue("thumbPagesReadahead", (int)GData::thumbPagesReadahead);
	GData::appSettings->setValue("thumbLayout", (int)GData::thumbsLayout);
	GData::appSettings->setValue("exitInsteadOfClose", (int)GData::exitInsteadOfClose);
	GData::appSettings->setValue("enableAnimations", (bool)GData::enableAnimations);
	GData::appSettings->setValue("exifRotationEnabled", (bool)GData::exifRotationEnabled);
	GData::appSettings->setValue("exifThumbRotationEnabled", (bool)GData::exifThumbRotationEnabled);
	GData::appSettings->setValue("reverseMouseBehavior", (bool)GData::reverseMouseBehavior);
	GData::appSettings->setValue("showHiddenFiles", (bool)GData::showHiddenFiles);
	GData::appSettings->setValue("wrapImageList", (bool)GData::wrapImageList);
	GData::appSettings->setValue("imageZoomFactor", (float)GData::imageZoomFactor);
	GData::appSettings->setValue("shouldMaximize", (bool)isMaximized());
	GData::appSettings->setValue("defaultSaveQuality", (int)GData::defaultSaveQuality);
	GData::appSettings->setValue("noEnlargeSmallThumb", (bool)GData::noEnlargeSmallThumb);
	GData::appSettings->setValue("slideShowDelay", (int)GData::slideShowDelay);
	GData::appSettings->setValue("slideShowRandom", (bool)GData::slideShowRandom);
	GData::appSettings->setValue("editToolBarVisible", (bool)editToolBarVisible);
	GData::appSettings->setValue("goToolBarVisible", (bool)goToolBarVisible);
	GData::appSettings->setValue("viewToolBarVisible", (bool)viewToolBarVisible);
	GData::appSettings->setValue("imageToolBarVisible", (bool)imageToolBarVisible);
	GData::appSettings->setValue("fsDockVisible", (bool)GData::fsDockVisible);
	GData::appSettings->setValue("iiDockVisible", (bool)GData::iiDockVisible);
	GData::appSettings->setValue("pvDockVisible", (bool)GData::pvDockVisible);
	GData::appSettings->setValue("startupDir", (int)GData::startupDir);
	GData::appSettings->setValue("specifiedStartDir", GData::specifiedStartDir);
	GData::appSettings->setValue("thumbsBackImage", GData::thumbsBackImage);
	GData::appSettings->setValue("lastDir", GData::startupDir == GData::rememberLastDir?
																		thumbView->currentViewDir: "");
	GData::appSettings->setValue("enableImageInfoFS", (bool)GData::enableImageInfoFS);
	GData::appSettings->setValue("showLabels", (bool)GData::showLabels);
	GData::appSettings->setValue("smallIcons", (bool)GData::smallIcons);
	GData::appSettings->setValue("LockDocks", (bool)GData::LockDocks);
	GData::appSettings->setValue("imageToolbarFullScreen", (bool)GData::imageToolbarFullScreen);

	/* Action shortcuts */
	GData::appSettings->beginGroup("Shortcuts");
	QMapIterator<QString, QAction *> scIter(GData::actionKeys);
	while (scIter.hasNext()) {
		scIter.next();
		GData::appSettings->setValue(scIter.key(), scIter.value()->shortcut().toString());
	}
	GData::appSettings->endGroup();

	/* External apps */
	GData::appSettings->beginGroup("ExternalApps");
	GData::appSettings->remove("");
	QMapIterator<QString, QString> eaIter(GData::externalApps);
	while (eaIter.hasNext()) {
		eaIter.next();
		GData::appSettings->setValue(eaIter.key(), eaIter.value());
	}
	GData::appSettings->endGroup();

	/* save bookmarks */
	int idx = 0;
	GData::appSettings->beginGroup("CopyMoveToPaths");
	GData::appSettings->remove("");
	QSetIterator<QString> pathsIter(GData::bookmarkPaths);
	while (pathsIter.hasNext()) {
		GData::appSettings->setValue("path" + QString::number(++idx), pathsIter.next());
	}
	GData::appSettings->endGroup();
}

void Phototonic::readSettings()
{
	initComplete = false;
	needThumbsRefresh = false;

	if (!GData::appSettings->contains("thumbsZoomVal")) {
		resize(800, 600);
		GData::appSettings->setValue("thumbsSortFlags", (int)0);
		GData::appSettings->setValue("thumbsZoomVal", (int)150);
		GData::appSettings->setValue("isFullScreen", (bool)false);
		GData::appSettings->setValue("backgroundColor", QColor(25, 25, 25));
		GData::appSettings->setValue("backgroundThumbColor", QColor(200, 200, 200));
		GData::appSettings->setValue("textThumbColor", QColor(25, 25, 25));
		GData::appSettings->setValue("thumbSpacing", (int)10);
		GData::appSettings->setValue("thumbPagesReadahead", (int)2);
		GData::appSettings->setValue("thumbLayout", (int)GData::thumbsLayout);
		GData::appSettings->setValue("zoomOutFlags", (int)1);
		GData::appSettings->setValue("zoomInFlags", (int)0);
		GData::appSettings->setValue("wrapImageList", (bool)false);
		GData::appSettings->setValue("exitInsteadOfClose", (int)0);
		GData::appSettings->setValue("imageZoomFactor", (float)1.0);
		GData::appSettings->setValue("defaultSaveQuality", (int)90);
		GData::appSettings->setValue("noEnlargeSmallThumb", (bool)true);
		GData::appSettings->setValue("enableAnimations", (bool)true);
		GData::appSettings->setValue("exifRotationEnabled", (bool)true);
		GData::appSettings->setValue("exifThumbRotationEnabled", (bool)false);
		GData::appSettings->setValue("reverseMouseBehavior", (bool)false);
		GData::appSettings->setValue("showHiddenFiles", (bool)false);
		GData::appSettings->setValue("slideShowDelay", (int)5);
		GData::appSettings->setValue("slideShowRandom", (bool)false);
		GData::appSettings->setValue("editToolBarVisible", (bool)true);
		GData::appSettings->setValue("goToolBarVisible", (bool)true);
		GData::appSettings->setValue("viewToolBarVisible", (bool)true);
		GData::appSettings->setValue("imageToolBarVisible", (bool)false);
		GData::appSettings->setValue("fsDockVisible", (bool)true);
		GData::appSettings->setValue("bmDockVisible", (bool)true);
		GData::appSettings->setValue("iiDockVisible", (bool)true);
		GData::appSettings->setValue("pvDockVisible", (bool)true);
		GData::appSettings->setValue("enableImageInfoFS", (bool)false);
		GData::appSettings->setValue("showLabels", (bool)true);
		GData::appSettings->setValue("smallIcons", (bool)false);
		GData::appSettings->setValue("LockDocks", (bool)true);
		GData::appSettings->setValue("imageToolbarFullScreen", (bool)false);
		GData::bookmarkPaths.insert(QDir::homePath());
	}

	GData::backgroundColor = GData::appSettings->value("backgroundColor").value<QColor>();
	GData::exitInsteadOfClose = GData::appSettings->value("exitInsteadOfClose").toBool();
	GData::enableAnimations = GData::appSettings->value("enableAnimations").toBool();
	GData::exifRotationEnabled = GData::appSettings->value("exifRotationEnabled").toBool();
	GData::exifThumbRotationEnabled = GData::appSettings->value("exifThumbRotationEnabled").toBool();
	GData::reverseMouseBehavior = GData::appSettings->value("reverseMouseBehavior").toBool();
	GData::showHiddenFiles = GData::appSettings->value("showHiddenFiles").toBool();
	GData::wrapImageList = GData::appSettings->value("wrapImageList").toBool();
	GData::imageZoomFactor = GData::appSettings->value("imageZoomFactor").toFloat();
	GData::zoomOutFlags = GData::appSettings->value("zoomOutFlags").toInt();
	GData::zoomInFlags = GData::appSettings->value("zoomInFlags").toInt();
	GData::rotation = 0;
	GData::keepTransform = false;
	shouldMaximize = GData::appSettings->value("shouldMaximize").toBool();
	GData::flipH = false;
	GData::flipV = false;
	GData::defaultSaveQuality = GData::appSettings->value("defaultSaveQuality").toInt();
	GData::noEnlargeSmallThumb = GData::appSettings->value("noEnlargeSmallThumb").toBool();
	GData::slideShowDelay = GData::appSettings->value("slideShowDelay").toInt();
	GData::slideShowRandom = GData::appSettings->value("slideShowRandom").toBool();
	GData::slideShowActive = false;
	editToolBarVisible = GData::appSettings->value("editToolBarVisible").toBool();
	goToolBarVisible = GData::appSettings->value("goToolBarVisible").toBool();
	viewToolBarVisible = GData::appSettings->value("viewToolBarVisible").toBool();
	imageToolBarVisible = GData::appSettings->value("imageToolBarVisible").toBool();
	GData::fsDockVisible = GData::appSettings->value("fsDockVisible").toBool();
	GData::bmDockVisible = GData::appSettings->value("bmDockVisible").toBool();
	GData::iiDockVisible = GData::appSettings->value("iiDockVisible").toBool();
	GData::pvDockVisible = GData::appSettings->value("pvDockVisible").toBool();
	GData::startupDir = (GData::StartupDir)GData::appSettings->value("startupDir").toInt();
	GData::specifiedStartDir = GData::appSettings->value("specifiedStartDir").toString();
	GData::thumbsBackImage = GData::appSettings->value("thumbsBackImage").toString();
	GData::enableImageInfoFS = GData::appSettings->value("enableImageInfoFS").toBool();
	GData::showLabels = GData::appSettings->value("showLabels").toBool();
	GData::smallIcons = GData::appSettings->value("smallIcons").toBool();
	GData::LockDocks = GData::appSettings->value("LockDocks").toBool();
	GData::imageToolbarFullScreen = GData::appSettings->value("imageToolbarFullScreen").toBool();

	GData::appSettings->beginGroup("ExternalApps");
	QStringList extApps = GData::appSettings->childKeys();
	for (int i = 0; i < extApps.size(); ++i)
	{
		GData::externalApps[extApps.at(i)] = GData::appSettings->value(extApps.at(i)).toString();
	}
	GData::appSettings->endGroup();

	GData::appSettings->beginGroup("CopyMoveToPaths");
	QStringList paths = GData::appSettings->childKeys();
	for (int i = 0; i < paths.size(); ++i)
	{
		GData::bookmarkPaths.insert(GData::appSettings->value(paths.at(i)).toString());
	}
	GData::appSettings->endGroup();
}

void Phototonic::setupDocks()
{
	pvDock = new QDockWidget(tr("Viewer"), this);
	pvDock->setObjectName("Viewer");

	imageViewContainer = new QVBoxLayout;
	imageViewContainer->setContentsMargins(0, 0, 0, 0);
	imageViewContainer->addWidget(imageView);
	QWidget *imageViewContainerWidget = new QWidget;
	imageViewContainerWidget->setLayout(imageViewContainer);
	
	pvDock->setWidget(imageViewContainerWidget);
	connect(pvDock->toggleViewAction(), SIGNAL(triggered()), this, SLOT(setPvDockVisibility()));	
	connect(pvDock, SIGNAL(visibilityChanged(bool)), this, SLOT(setPvDockVisibility()));	
	addDockWidget(Qt::RightDockWidgetArea, pvDock);
	addDockWidget(Qt::RightDockWidgetArea, iiDock);

	QAction *docksNToolbarsAct = viewMenu->insertMenu(refreshAction, createPopupMenu());
	docksNToolbarsAct->setText(tr("Docks and Toolbars"));

	fsDockOrigWidget = fsDock->titleBarWidget();
	bmDockOrigWidget = bmDock->titleBarWidget();
	iiDockOrigWidget = iiDock->titleBarWidget();
	pvDockOrigWidget = pvDock->titleBarWidget();
	fsDockEmptyWidget = new QWidget;
	bmDockEmptyWidget = new QWidget;
	iiDockEmptyWidget = new QWidget;
	pvDockEmptyWidget = new QWidget;
	lockDocks();

	setDockOptions(QMainWindow::AllowNestedDocks);
}

void Phototonic::lockDocks()
{
	if (initComplete)
		GData::LockDocks = actLockDocks->isChecked();

	if (GData::LockDocks) {
		fsDock->setTitleBarWidget(fsDockEmptyWidget);
		bmDock->setTitleBarWidget(bmDockEmptyWidget);
		iiDock->setTitleBarWidget(iiDockEmptyWidget);
		pvDock->setTitleBarWidget(pvDockEmptyWidget);
	} else {
		fsDock->setTitleBarWidget(fsDockOrigWidget);
		bmDock->setTitleBarWidget(bmDockOrigWidget);
		iiDock->setTitleBarWidget(iiDockOrigWidget);
		pvDock->setTitleBarWidget(pvDockOrigWidget);
	}
}

QMenu *Phototonic::createPopupMenu()
{
	QMenu *testMenu = QMainWindow::createPopupMenu();
	testMenu->addSeparator();
	testMenu->addAction(actSmallIcons);
	testMenu->addAction(actLockDocks);
	return testMenu;
}

void Phototonic::loadShortcuts()
{
	// Add customizable key shortcut actions
	GData::actionKeys[thumbsGoTopAct->text()] = thumbsGoTopAct;
	GData::actionKeys[thumbsGoBottomAct->text()] = thumbsGoBottomAct;
	GData::actionKeys[closeImageAct->text()] = closeImageAct;
	GData::actionKeys[fullScreenAct->text()] = fullScreenAct;
	GData::actionKeys[settingsAction->text()] = settingsAction;
	GData::actionKeys[exitAction->text()] = exitAction;
	GData::actionKeys[thumbsZoomInAct->text()] = thumbsZoomInAct;
	GData::actionKeys[thumbsZoomOutAct->text()] = thumbsZoomOutAct;
	GData::actionKeys[cutAction->text()] = cutAction;
	GData::actionKeys[copyAction->text()] = copyAction;
	GData::actionKeys[nextImageAction->text()] = nextImageAction;
	GData::actionKeys[prevImageAction->text()] = prevImageAction;
	GData::actionKeys[deleteAction->text()] = deleteAction;
	GData::actionKeys[saveAction->text()] = saveAction;
	GData::actionKeys[saveAsAction->text()] = saveAsAction;
	GData::actionKeys[keepTransformAct->text()] = keepTransformAct;
	GData::actionKeys[keepZoomAct->text()] = keepZoomAct;
	GData::actionKeys[showClipboardAction->text()] = showClipboardAction;
	GData::actionKeys[copyImageAction->text()] = copyImageAction;
	GData::actionKeys[pasteImageAction->text()] = pasteImageAction;
	GData::actionKeys[renameAction->text()] = renameAction;
	GData::actionKeys[refreshAction->text()] = refreshAction;
	GData::actionKeys[pasteAction->text()] = pasteAction;
	GData::actionKeys[goBackAction->text()] = goBackAction;
	GData::actionKeys[goFrwdAction->text()] = goFrwdAction;
	GData::actionKeys[slideShowAction->text()] = slideShowAction;
	GData::actionKeys[firstImageAction->text()] = firstImageAction;
	GData::actionKeys[lastImageAction->text()] = lastImageAction;
	GData::actionKeys[randomImageAction->text()] = randomImageAction;
	GData::actionKeys[openAction->text()] = openAction;
	GData::actionKeys[zoomOutAct->text()] = zoomOutAct;
	GData::actionKeys[zoomInAct->text()] = zoomInAct;
	GData::actionKeys[resetZoomAct->text()] = resetZoomAct;
	GData::actionKeys[origZoomAct->text()] = origZoomAct;
	GData::actionKeys[rotateLeftAct->text()] = rotateLeftAct;
	GData::actionKeys[rotateRightAct->text()] = rotateRightAct;
	GData::actionKeys[freeRotateLeftAct->text()] = freeRotateLeftAct;
	GData::actionKeys[freeRotateRightAct->text()] = freeRotateRightAct;
	GData::actionKeys[flipHAct->text()] = flipHAct;
	GData::actionKeys[flipVAct->text()] = flipVAct;
	GData::actionKeys[cropAct->text()] = cropAct;
	GData::actionKeys[cropToSelectionAct->text()] = cropToSelectionAct;
	GData::actionKeys[colorsAct->text()] = colorsAct;
	GData::actionKeys[mirrorDisabledAct->text()] = mirrorDisabledAct;
	GData::actionKeys[mirrorDualAct->text()] = mirrorDualAct;
	GData::actionKeys[mirrorTripleAct->text()] = mirrorTripleAct;
	GData::actionKeys[mirrorVDualAct->text()] = mirrorVDualAct;
	GData::actionKeys[mirrorQuadAct->text()] = mirrorQuadAct;
	GData::actionKeys[moveDownAct->text()] = moveDownAct;
	GData::actionKeys[moveUpAct->text()] = moveUpAct;
	GData::actionKeys[moveRightAct->text()] = moveRightAct;
	GData::actionKeys[moveLeftAct->text()] = moveLeftAct;
	GData::actionKeys[copyToAction->text()] = copyToAction;
	GData::actionKeys[moveToAction->text()] = moveToAction;
	GData::actionKeys[goUpAction->text()] = goUpAction;
	GData::actionKeys[resizeAct->text()] = resizeAct;
	GData::actionKeys[filterImagesFocusAct->text()] = filterImagesFocusAct;
	GData::actionKeys[setPathFocusAct->text()] = setPathFocusAct;
	GData::actionKeys[keepTransformAct->text()] = keepTransformAct;
	
	GData::appSettings->beginGroup("Shortcuts");
	QStringList groupKeys = GData::appSettings->childKeys();

	if (groupKeys.size())
	{
		for (int i = 0; i < groupKeys.size(); ++i)
		{
			if (GData::actionKeys.value(groupKeys.at(i)))
				GData::actionKeys.value(groupKeys.at(i))->setShortcut
											(GData::appSettings->value(groupKeys.at(i)).toString());
		}
	}
	else
	{
		thumbsGoTopAct->setShortcut(QKeySequence("Ctrl+Home"));
		thumbsGoBottomAct->setShortcut(QKeySequence("Ctrl+End"));
		closeImageAct->setShortcut(Qt::Key_Escape);
		fullScreenAct->setShortcut(QKeySequence("F"));
		settingsAction->setShortcut(QKeySequence("P"));
		exitAction->setShortcut(QKeySequence("Ctrl+Q"));
		cutAction->setShortcut(QKeySequence("Ctrl+X"));
		copyAction->setShortcut(QKeySequence("Ctrl+C"));
		deleteAction->setShortcut(QKeySequence("Del"));
		saveAction->setShortcut(QKeySequence("Ctrl+S"));
		copyImageAction->setShortcut(QKeySequence("Ctrl+Shift+C"));
		pasteImageAction->setShortcut(QKeySequence("Ctrl+Shift+V"));
		renameAction->setShortcut(QKeySequence("F2"));
		refreshAction->setShortcut(QKeySequence("F5"));
		pasteAction->setShortcut(QKeySequence("Ctrl+V"));
		goBackAction->setShortcut(QKeySequence("Alt+Left"));
		goFrwdAction->setShortcut(QKeySequence("Alt+Right"));
		goUpAction->setShortcut(QKeySequence("Alt+Up"));
		slideShowAction->setShortcut(QKeySequence("W"));
		nextImageAction->setShortcut(QKeySequence("PgDown"));
		prevImageAction->setShortcut(QKeySequence("PgUp"));
		firstImageAction->setShortcut(QKeySequence("Home"));
		lastImageAction->setShortcut(QKeySequence("End"));
		randomImageAction->setShortcut(QKeySequence("R"));
		openAction->setShortcut(QKeySequence("Return"));
		zoomOutAct->setShortcut(QKeySequence("Alt+Z"));
		zoomInAct->setShortcut(QKeySequence("Z"));
		resetZoomAct->setShortcut(QKeySequence("Ctrl+Z"));
		origZoomAct->setShortcut(QKeySequence("Shift+Z"));
		rotateLeftAct->setShortcut(QKeySequence("Ctrl+Left"));
		rotateRightAct->setShortcut(QKeySequence("Ctrl+Right"));
		freeRotateLeftAct->setShortcut(QKeySequence("Ctrl+Shift+Left"));
		freeRotateRightAct->setShortcut(QKeySequence("Ctrl+Shift+Right"));
		flipHAct->setShortcut(QKeySequence("Ctrl+Down"));
		flipVAct->setShortcut(QKeySequence("Ctrl+Up"));
		cropAct->setShortcut(QKeySequence("Ctrl+G"));
		cropToSelectionAct->setShortcut(QKeySequence("Ctrl+R"));
		colorsAct->setShortcut(QKeySequence("C"));
		mirrorDisabledAct->setShortcut(QKeySequence("Ctrl+1"));
		mirrorDualAct->setShortcut(QKeySequence("Ctrl+2"));
		mirrorTripleAct->setShortcut(QKeySequence("Ctrl+3"));
		mirrorVDualAct->setShortcut(QKeySequence("Ctrl+4"));
		mirrorQuadAct->setShortcut(QKeySequence("Ctrl+5"));
		moveDownAct->setShortcut(QKeySequence("Down"));
		moveUpAct->setShortcut(QKeySequence("Up"));
		moveLeftAct->setShortcut(QKeySequence("Left"));
		moveRightAct->setShortcut(QKeySequence("Right"));
		copyToAction->setShortcut(QKeySequence("Ctrl+Y"));
		moveToAction->setShortcut(QKeySequence("Ctrl+M"));
		resizeAct->setShortcut(QKeySequence("Ctrl+I"));
		filterImagesFocusAct->setShortcut(QKeySequence("Ctrl+F"));
		setPathFocusAct->setShortcut(QKeySequence("Ctrl+L"));
		keepTransformAct->setShortcut(QKeySequence("Ctrl+K"));
	}
		
	GData::appSettings->endGroup();
}

void Phototonic::closeEvent(QCloseEvent *event)
{
	thumbView->abort();
	writeSettings();
	if (!QApplication::clipboard()->image().isNull())
		QApplication::clipboard()->clear();
	event->accept();
}

void Phototonic::setStatus(QString state)
{
	stateLabel->setText("    " + state + "    ");
}

void Phototonic::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (interfaceDisabled)
		return;

	if (event->button() == Qt::LeftButton) {
		if (GData::layoutMode == imageViewIdx) {
			if (GData::reverseMouseBehavior)
			{
				fullScreenAct->setChecked(!(fullScreenAct->isChecked()));
				toggleFullScreen();
				event->accept();
			}
			else if (closeImageAct->isEnabled())
			{
				hideViewer();
				event->accept();
			}
		} else {
			if (QApplication::focusWidget() == imageView->scrlArea) {
				openOp();
			}
		}
	}
}

void Phototonic::mousePressEvent(QMouseEvent *event)
{
	if (interfaceDisabled)
		return;

	if (GData::layoutMode == imageViewIdx) {
		if (event->button() == Qt::MiddleButton) {

			if (event->modifiers() == Qt::ShiftModifier) {
				origZoom();
				event->accept();
				return;
			}
			if (event->modifiers() == Qt::ControlModifier) {
				resetZoom();
				event->accept();
				return;
			}

			if (GData::reverseMouseBehavior && closeImageAct->isEnabled()) {
				hideViewer();
				event->accept();
			} else {
				fullScreenAct->setChecked(!(fullScreenAct->isChecked()));
				toggleFullScreen();
				event->accept();
			}
		}
	} else {
		if (QApplication::focusWidget() == imageView->scrlArea) {
			if (event->button() == Qt::MiddleButton && GData::reverseMouseBehavior) {
				openOp();
			}
		}
	}
}

void Phototonic::newImage()
{
	if (GData::layoutMode == thumbViewIdx)
		showViewer();

	imageView->loadImage("");
}

void Phototonic::setDocksVisibility(bool visible)
{
	if (!visible) {
		fsDock->setMaximumHeight(fsDock->height());
		bmDock->setMaximumHeight(bmDock->height());
		iiDock->setMaximumHeight(iiDock->height());
		pvDock->setMaximumHeight(pvDock->height());
		fsDock->setMaximumWidth(fsDock->width());
		bmDock->setMaximumWidth(bmDock->width());
		iiDock->setMaximumWidth(iiDock->width());
		pvDock->setMaximumWidth(pvDock->width());
	}

	fsDock->setVisible(visible? GData::fsDockVisible : false);
	bmDock->setVisible(visible? GData::fsDockVisible : false);
	iiDock->setVisible(visible? GData::iiDockVisible : false);
	pvDock->setVisible(visible? GData::pvDockVisible : false);

	menuBar()->setVisible(visible);
	menuBar()->setDisabled(!visible);
	statusBar()->setVisible(visible);

	editToolBar->setVisible(visible? editToolBarVisible : false);
	goToolBar->setVisible(visible? goToolBarVisible : false);
	viewToolBar->setVisible(visible? viewToolBarVisible : false);
	imageToolBar->setVisible(visible? imageToolBarVisible : GData::imageToolbarFullScreen);

	setContextMenuPolicy(Qt::PreventContextMenu);
}

void Phototonic::openOp()
{
	if (GData::layoutMode == imageViewIdx)
	{
		hideViewer();
		return;
	}
	
	if (QApplication::focusWidget() == fsTree)
	{
		goSelectedDir(fsTree->getCurrentIndex());
		return;
	}
	else if (QApplication::focusWidget() == thumbView 
				|| QApplication::focusWidget() == imageView->scrlArea)
	{
		QModelIndex idx;
		QModelIndexList indexesList = thumbView->selectionModel()->selectedIndexes();
		if (indexesList.size() > 0)
			idx = indexesList.first();
		else
		{
			if (thumbView->thumbViewModel->rowCount() == 0)
			{
				setStatus(tr("No images"));
				return;
			}

			idx = thumbView->thumbViewModel->indexFromItem(thumbView->thumbViewModel->item(0));
			thumbView->selectionModel()->select(idx, QItemSelectionModel::Toggle);
			thumbView->setCurrentRow(0);
		}

		loadImagefromThumb(idx);
		return;
	}
	else if (QApplication::focusWidget() == filterBar)
	{
		setThumbsFilter();
		return;
	}
	else if (QApplication::focusWidget() == pathBar)
	{
		goPathBarDir();
		return;
	}
}

void Phototonic::setEditToolBarVisibility()
{
	editToolBarVisible = editToolBar->isVisible();
}

void Phototonic::setGoToolBarVisibility()
{
	goToolBarVisible = goToolBar->isVisible();
}

void Phototonic::setViewToolBarVisibility()
{
	viewToolBarVisible = viewToolBar->isVisible();
}

void Phototonic::setImageToolBarVisibility()
{
	imageToolBarVisible = imageToolBar->isVisible();
}

void Phototonic::setFsDockVisibility()
{
	if (GData::layoutMode == imageViewIdx)
		return;

	GData::fsDockVisible = fsDock->isVisible();
}

void Phototonic::setBmDockVisibility()
{
	if (GData::layoutMode == imageViewIdx)
		return;

	GData::bmDockVisible = bmDock->isVisible();
}

void Phototonic::setIiDockVisibility()
{
	if (GData::layoutMode == imageViewIdx)
		return;

	GData::iiDockVisible = iiDock->isVisible();
}

void Phototonic::setPvDockVisibility()
{
	if (GData::layoutMode == imageViewIdx)
		return;

	GData::pvDockVisible = pvDock->isVisible();
}

void Phototonic::showViewer()
{
	if (GData::layoutMode == thumbViewIdx) {
		GData::layoutMode = imageViewIdx;
		GData::appSettings->setValue("Geometry", saveGeometry());
		GData::appSettings->setValue("WindowState", saveState());

		imageViewContainer->removeWidget(imageView);
		mainLayout->addWidget(imageView);
		imageView->setVisible(true);
		thumbView->setVisible(false);
		setDocksVisibility(false);
		if (GData::isFullScreen == true) {
			shouldMaximize = isMaximized();
			showFullScreen();
			imageView->setCursorHiding(true);
			imageView->infoLabel->setVisible(GData::enableImageInfoFS);
		}
		imageView->adjustSize();
	}
}

void Phototonic::showBusyStatus(bool busy)
{
	static int busyStatus = 0;

	if (busy)
		++busyStatus;
	else
		--busyStatus;

	if (busyStatus > 0) {
		busyMovie->start();
		busyLabel->setVisible(true);
	} else {
		busyLabel->setVisible(false);
		busyMovie->stop();
		busyStatus = 0;
	}
}

void Phototonic::loadImagefromThumb(const QModelIndex &idx)
{
	thumbView->setCurrentRow(idx.row());
	showViewer();
	imageView->loadImage(thumbView->thumbViewModel->item(idx.row())->data(thumbView->FileNameRole).toString());
	thumbView->setImageviewWindowTitle();
}

void Phototonic::updateViewerImageBySelection(const QItemSelection&)
{
	if (!pvDock->isVisible() || GData::layoutMode == imageViewIdx)
		return;
			
	QModelIndexList indexesList = thumbView->selectionModel()->selectedIndexes();
	if (indexesList.size() == 1) {
		QString ImagePath = thumbView->thumbViewModel->item(indexesList.first().row())->data
																(thumbView->FileNameRole).toString();
		imageView->loadImage(ImagePath);
		thumbView->setCurrentRow(indexesList.first().row());
	} else {
		QString ImagePath(":/images/no_image.png");
		imageView->loadImage(ImagePath);
	}
}

void Phototonic::loadImagefromCli()
{
	QFile imageFile(cliFileName);
	if(!imageFile.exists()) 
	{
		QMessageBox msgBox;
		msgBox.critical(this, tr("Error"), tr("Failed to open file \"%1\": file not found.").arg(cliFileName));
		cliFileName = "";
		return;
	}

	showViewer();
	imageView->loadImage(cliFileName);
	setWindowTitle(cliFileName + " - Phototonic");
}

void Phototonic::slideShow()
{
	if (GData::slideShowActive)
	{
		GData::slideShowActive = false;
		slideShowAction->setText(tr("Slide Show"));
		imageView->setFeedback(tr("Slide show stopped"));

		SlideShowTimer->stop();
		delete SlideShowTimer;
		slideShowAction->setIcon(QIcon::fromTheme("media-playback-start", QIcon(":/images/play.png")));
	}
	else
	{
		if (thumbView->thumbViewModel->rowCount() <= 0)
			return;
	
		if (GData::layoutMode == thumbViewIdx)
		{
			QModelIndexList indexesList = thumbView->selectionModel()->selectedIndexes();
			if (indexesList.size() != 1)
				thumbView->setCurrentRow(0);
			else
				thumbView->setCurrentRow(indexesList.first().row());

			showViewer();
		}
	
		GData::slideShowActive = true;

		SlideShowTimer = new QTimer(this);
		connect(SlideShowTimer, SIGNAL(timeout()), this, SLOT(slideShowHandler()));
		SlideShowTimer->start(GData::slideShowDelay * 1000);

		slideShowAction->setText(tr("Stop Slide Show"));
		imageView->setFeedback(tr("Slide show started"));
		slideShowAction->setIcon(QIcon::fromTheme("media-playback-stop", QIcon(":/images/stop.png")));

		slideShowHandler();
	}
}

void Phototonic::slideShowHandler()
{
	if (GData::slideShowActive)
	{
		if (GData::slideShowRandom)
		{
			loadRandomImage();
		}
		else
		{
			int currentRow = thumbView->getCurrentRow();
			imageView->loadImage(thumbView->thumbViewModel->item(currentRow)->data(thumbView->FileNameRole).toString());
			thumbView->setImageviewWindowTitle();

			if (thumbView->getNextRow() > 0)
				thumbView->setCurrentRow(thumbView->getNextRow());
			else 
			{
				if (GData::wrapImageList)
					thumbView->setCurrentRow(0);
				else
					slideShow();
			}
		}
	}
}

void Phototonic::selectThumbByRow(int row)
{
	thumbView->setCurrentIndexByRow(row);
	thumbView->selectCurrentIndex();
}

void Phototonic::loadNextImage()
{
	if (thumbView->thumbViewModel->rowCount() <= 0)
		return;

	int nextRow = thumbView->getNextRow();
	if (nextRow < 0) 
	{
		if (GData::wrapImageList)
			nextRow = 0;
		else
			return;
	}

	imageView->loadImage(thumbView->thumbViewModel->item(nextRow)->data(thumbView->FileNameRole).toString());
	thumbView->setCurrentRow(nextRow);
	thumbView->setImageviewWindowTitle();

	if (GData::layoutMode == thumbViewIdx)
		selectThumbByRow(nextRow);
}

void Phototonic::loadPrevImage()
{
	if (thumbView->thumbViewModel->rowCount() <= 0)
		return;

	int prevRow = thumbView->getPrevRow();
	if (prevRow < 0) 
	{
		if (GData::wrapImageList)
			prevRow = thumbView->getLastRow();
		else
			return;
	}
	
	imageView->loadImage(thumbView->thumbViewModel->item(prevRow)->data(thumbView->FileNameRole).toString());
	thumbView->setCurrentRow(prevRow);
	thumbView->setImageviewWindowTitle();

	if (GData::layoutMode == thumbViewIdx)
		selectThumbByRow(prevRow);
}

void Phototonic::loadFirstImage()
{
	if (thumbView->thumbViewModel->rowCount() <= 0)
		return;

	imageView->loadImage(thumbView->thumbViewModel->item(0)->data(thumbView->FileNameRole).toString());
	thumbView->setCurrentRow(0);
	thumbView->setImageviewWindowTitle();

	if (GData::layoutMode == thumbViewIdx)
		selectThumbByRow(0);
}

void Phototonic::loadLastImage()
{
	if (thumbView->thumbViewModel->rowCount() <= 0)
		return;

	int lastRow = thumbView->getLastRow();
	imageView->loadImage(thumbView->thumbViewModel->item(lastRow)->data(thumbView->FileNameRole).toString());
	thumbView->setCurrentRow(lastRow);
	thumbView->setImageviewWindowTitle();

	if (GData::layoutMode == thumbViewIdx)
		selectThumbByRow(lastRow);
}

void Phototonic::loadRandomImage()
{
	if (thumbView->thumbViewModel->rowCount() <= 0)
		return;

	int randomRow = thumbView->getRandomRow();
	imageView->loadImage(thumbView->thumbViewModel->item(randomRow)->data(thumbView->FileNameRole).toString());
	thumbView->setCurrentRow(randomRow);
	thumbView->setImageviewWindowTitle();

	if (GData::layoutMode == thumbViewIdx)
		selectThumbByRow(randomRow);
}

void Phototonic::selectRecentThumb()
{
	if (thumbView->thumbViewModel->rowCount() > 0) 
	{
		if (thumbView->setCurrentIndexByName(thumbView->recentThumb))
			thumbView->selectCurrentIndex();
		else
		{
			if (thumbView->setCurrentIndexByRow(0))
				thumbView->selectCurrentIndex();
		}
	}
}

void Phototonic::setViewerKeyEventsEnabled(bool enabled)
{
	moveLeftAct->setEnabled(enabled);
	moveRightAct->setEnabled(enabled);
	moveUpAct->setEnabled(enabled);
	moveDownAct->setEnabled(enabled);
}

void Phototonic::updateIndexByViewerImage()
{
	if (thumbView->thumbViewModel->rowCount() > 0) {
		if (thumbView->setCurrentIndexByName(imageView->currentImageFullPath))
			thumbView->selectCurrentIndex();
	}
}

void Phototonic::hideViewer()
{
	if (cliImageLoaded && GData::exitInsteadOfClose) {
		close();
		return;
	}

	showBusyStatus(true);

	if (isFullScreen())	{
		showNormal();
		if (shouldMaximize)
			showMaximized();
		imageView->setCursorHiding(false);
		imageView->infoLabel->setVisible(false);
	}

	while (qApp->hasPendingEvents()) {
		QApplication::processEvents();
	}

	GData::layoutMode = thumbViewIdx;
	mainLayout->removeWidget(imageView);
	
	imageViewContainer->addWidget(imageView);
	setDocksVisibility(true);
	while (QApplication::overrideCursor()) {
		QApplication::restoreOverrideCursor();
	}

	if (GData::slideShowActive) {
		slideShow();
	}

	thumbView->setResizeMode(QListView::Fixed);
	thumbView->setVisible(true);
	while (qApp->hasPendingEvents()) {
		QApplication::processEvents();
	}
	setThumbviewWindowTitle();

	fsDock->setMaximumHeight(QWIDGETSIZE_MAX);
	bmDock->setMaximumHeight(QWIDGETSIZE_MAX);
	iiDock->setMaximumHeight(QWIDGETSIZE_MAX);
	pvDock->setMaximumHeight(QWIDGETSIZE_MAX);
	fsDock->setMaximumWidth(QWIDGETSIZE_MAX);
	bmDock->setMaximumWidth(QWIDGETSIZE_MAX);
	iiDock->setMaximumWidth(QWIDGETSIZE_MAX);
	pvDock->setMaximumWidth(QWIDGETSIZE_MAX);

	if (!cliFileName.isEmpty()) {
		cliFileName = "";
		if (!shouldMaximize) {
			restoreGeometry(GData::appSettings->value("Geometry").toByteArray());
		}
		restoreState(GData::appSettings->value("WindowState").toByteArray());
	}

	if (thumbView->thumbViewModel->rowCount() > 0) {
		if (thumbView->setCurrentIndexByName(imageView->currentImageFullPath))
			thumbView->selectCurrentIndex();
	}
	thumbView->setResizeMode(QListView::Adjust);

	if (needThumbsRefresh) {
		needThumbsRefresh = false;
		refreshThumbs(true);
	} else {
		thumbView->loadVisibleThumbs();
	}

	thumbView->setFocus(Qt::OtherFocusReason);
	showBusyStatus(false);
	setContextMenuPolicy(Qt::DefaultContextMenu);
}

void Phototonic::goBottom()
{
	thumbView->scrollToBottom();
}

void Phototonic::goTop()
{
	thumbView->scrollToTop();
}

void Phototonic::dropOp(Qt::KeyboardModifiers keyMods, bool dirOp, QString cpMvDirPath)
{
	QApplication::restoreOverrideCursor();
	GData::copyOp = (keyMods == Qt::ControlModifier);
	QMessageBox msgBox;
	QString destDir;

	if (QObject::sender() == fsTree) {
		destDir = getSelectedPath();
	} else if (QObject::sender() == bookmarks) {
		if (bookmarks->currentItem()) {
			destDir = bookmarks->currentItem()->toolTip(0);
		} else {
			addBookmark(cpMvDirPath);
			return;
		}
	} else {
		// Unknown sender
		return;		
	}
	
	if (!isValidPath(destDir)) {
		msgBox.critical(this, tr("Error"), tr("Can not move or copy images to this folder."));
		selectCurrentViewDir();
		return;
	}
	
	if (destDir == 	thumbView->currentViewDir) {
		msgBox.critical(this, tr("Error"), tr("Destination folder is same as source."));
		return;
	}

	if (dirOp) {
		QString dirOnly = 
			cpMvDirPath.right(cpMvDirPath.size() - cpMvDirPath.lastIndexOf(QDir::separator()) - 1);

		QString question = tr("Move \"%1\" to \"%2\"?").arg(dirOnly).arg(destDir);
		int ret = QMessageBox::question(this, tr("Move folder"), question,
							QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

		if (ret == QMessageBox::Yes) {
			QFile dir(cpMvDirPath);
			bool ok = dir.rename(destDir + QDir::separator() + dirOnly);
			if (!ok) {
				QMessageBox msgBox;
				msgBox.critical(this, tr("Error"), tr("Failed to move folder."));
			}
			setStatus(tr("Folder moved"));
		}
	} else {
		CpMvDialog *cpMvdialog = new CpMvDialog(this);
		GData::copyCutIdxList = thumbView->selectionModel()->selectedIndexes();
		cpMvdialog->exec(thumbView, destDir, false);
		QString state = QString((GData::copyOp? tr("Copied") : tr("Moved")) + " " +
							tr("%n image(s)", "", cpMvdialog->nfiles));
		setStatus(state);
		delete(cpMvdialog);
	}

	thumbView->loadVisibleThumbs();
}

void Phototonic::selectCurrentViewDir()
{
	QModelIndex idx = fsTree->fsModel->index(thumbView->currentViewDir); 
	if (idx.isValid())
	{
		fsTree->expand(idx);
		fsTree->setCurrentIndex(idx);
	}
}

void Phototonic::checkDirState(const QModelIndex &, int, int)
{
	if (!initComplete) {
		return;
	}

	if (thumbView->busy)
	{
		thumbView->abort();
	}

	if (!QDir().exists(thumbView->currentViewDir))
	{
		thumbView->currentViewDir = "";
		QTimer::singleShot(0, this, SLOT(reloadThumbsSlot()));
	}
}

void Phototonic::recordHistory(QString dir)
{
	if (!needHistoryRecord)
	{
		needHistoryRecord = true;
		return;
	}

	if (pathHistory.size() && dir == pathHistory.at(currentHistoryIdx))
		return;
		
	pathHistory.insert(++currentHistoryIdx, dir);

	// Need to clear irrelevant items from list
	if (currentHistoryIdx != pathHistory.size() - 1)
	{	
		goFrwdAction->setEnabled(false);
		for (int i = pathHistory.size() - 1; i > currentHistoryIdx ; --i)
		{
			pathHistory.removeAt(i);
		}
	}
}

void Phototonic::reloadThumbsSlot()
{
	if (thumbView->busy || !initComplete)
	{	
		thumbView->abort();
		QTimer::singleShot(0, this, SLOT(reloadThumbsSlot()));
		return;
	}

	if (thumbView->currentViewDir == "")
	{
		thumbView->currentViewDir = getSelectedPath();
		if (thumbView->currentViewDir == "")
			return;
	}

	QDir checkPath(thumbView->currentViewDir);
	if (!checkPath.exists() || !checkPath.isReadable())
	{
		QMessageBox msgBox;
		msgBox.critical(this, tr("Error"), tr("Failed to open folder:") + " " + thumbView->currentViewDir);
		return;
	}

	thumbView->infoView->clear();
	if (GData::layoutMode == thumbViewIdx && pvDock->isVisible()) {
		QString ImagePath(":/images/no_image.png");
		imageView->loadImage(ImagePath);
	}

	pathBar->setText(thumbView->currentViewDir);
	recordHistory(thumbView->currentViewDir);
	if (currentHistoryIdx > 0)
		goBackAction->setEnabled(true);

	if (GData::layoutMode == thumbViewIdx)
	{
		setThumbviewWindowTitle();
	}

	thumbView->busy = true;

	if (findDupesAction->isChecked()) {
		thumbView->loadDuplicates();
	} else {
		thumbView->load();
	}
}

void Phototonic::setThumbviewWindowTitle()
{
	if (findDupesAction->isChecked())
		setWindowTitle(tr("Duplicate images in %1").arg(thumbView->currentViewDir) + " - Phototonic");
	else
		setWindowTitle(thumbView->currentViewDir + " - Phototonic");
}

void Phototonic::renameDir()
{
	QModelIndexList selectedDirs = fsTree->selectionModel()->selectedRows();
	QFileInfo dirInfo = QFileInfo(fsTree->fsModel->filePath(selectedDirs[0]));

	bool ok;
	QString title = tr("Rename") + " " + dirInfo.completeBaseName();
	QString newDirName = QInputDialog::getText(this, title, 
							tr("New name:"), QLineEdit::Normal, dirInfo.completeBaseName(), &ok);

	if (!ok)
	{
		selectCurrentViewDir();
		return;
	}

	if(newDirName.isEmpty())
	{
		QMessageBox msgBox;
		msgBox.critical(this, tr("Error"), tr("Invalid name entered."));
		selectCurrentViewDir();
		return;
	}

	QFile dir(dirInfo.absoluteFilePath());
	QString newFullPathName = dirInfo.absolutePath() + QDir::separator() + newDirName;
	ok = dir.rename(newFullPathName);
	if (!ok)
	{
		QMessageBox msgBox;
		msgBox.critical(this, tr("Error"), tr("Failed to rename folder."));
		selectCurrentViewDir();
		return;
	}

	if (thumbView->currentViewDir == dirInfo.absoluteFilePath()) 
		fsTree->setCurrentIndex(fsTree->fsModel->index(newFullPathName));
	else
		selectCurrentViewDir();
}

void Phototonic::rename()
{
	if (QApplication::focusWidget() == fsTree) {
		renameDir();
		return;
	}

	if (GData::layoutMode == imageViewIdx) {

		if (imageView->isNewImage()) 	{
			showNewImageWarning(this);
			return;
		}
	
		if (thumbView->thumbViewModel->rowCount() > 0) {
			if (thumbView->setCurrentIndexByName(imageView->currentImageFullPath))
				thumbView->selectCurrentIndex();
		}
	}

	QString selectedImageFileName = thumbView->getSingleSelectionFilename();
	if (selectedImageFileName.isEmpty()) {
		setStatus(tr("Invalid selection"));
		return;
	}

	bool ok;

	QString title = tr("Rename Image");
	QString newImageName = QInputDialog::getText(this,
									title, tr("Enter a new name for \"%1\":")
									.arg(QFileInfo(selectedImageFileName).fileName())
									+ "\t\t\t",
									QLineEdit::Normal,
									QFileInfo(selectedImageFileName).completeBaseName(),
									&ok);

	if (!ok) {
		return;
	}

	if(newImageName.isEmpty()) {
		QMessageBox msgBox;
		msgBox.critical(this, tr("Error"), tr("No name entered."));
		return;
	}

	newImageName += "." + QFileInfo(selectedImageFileName).suffix();
	QString currnetFilePath = selectedImageFileName;
	QFile currentFile(currnetFilePath);

	QString newImageFullPath = thumbView->currentViewDir;
	if (newImageFullPath.right(1) == QDir::separator()) {
		newImageFullPath +=  newImageName;
	} else {
		newImageFullPath += QDir::separator() + newImageName;
	}
	
	ok = currentFile.rename(newImageFullPath);
	if (ok) {
		QModelIndexList indexesList = thumbView->selectionModel()->selectedIndexes();
		thumbView->thumbViewModel->item(
					indexesList.first().row())->setData(newImageFullPath, thumbView->FileNameRole);

		if (GData::thumbsLayout != ThumbView::Squares) {
			thumbView->thumbViewModel->item(
					indexesList.first().row())->setData(newImageName, Qt::DisplayRole);
		}

		if (GData::layoutMode == imageViewIdx) {
			thumbView->setImageviewWindowTitle();
			imageView->setInfo(newImageName);
			imageView->currentImageFullPath = newImageFullPath;
		}
	} else {
		QMessageBox msgBox;
		msgBox.critical(this, tr("Error"), tr("Failed to rename image."));
	}
}

void Phototonic::deleteDir()
{
	bool ok = true;
	QModelIndexList selectedDirs = fsTree->selectionModel()->selectedRows();
	QString deletePath = fsTree->fsModel->filePath(selectedDirs[0]);
	QModelIndex idxAbove = fsTree->indexAbove(selectedDirs[0]);
	QFileInfo dirInfo = QFileInfo(deletePath);
	QString question = tr("Permanently delete \"%1\" and all of its contents?").arg(dirInfo.completeBaseName());

	QMessageBox msgBox;
	msgBox.setText(question);
	msgBox.setWindowTitle(tr("Delete folder"));
	msgBox.setIcon(QMessageBox::Warning);
	msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
	msgBox.setDefaultButton(QMessageBox::Cancel);
	msgBox.setButtonText(QMessageBox::Yes, tr("Yes"));  
    msgBox.setButtonText(QMessageBox::Cancel, tr("Cancel"));  
	int ret = msgBox.exec();

	if (ret == QMessageBox::Yes)
		ok = removeDirOp(deletePath);
	else
	{
		selectCurrentViewDir();
		return;
	}

	if (!ok)
	{
		QMessageBox msgBox;
		msgBox.critical(this, tr("Error"), tr("Failed to delete folder."));
		selectCurrentViewDir();
	}

	QString state = QString(tr("Removed \"%1\"").arg(deletePath));
	setStatus(state);

	if (thumbView->currentViewDir == deletePath) 
	{
		if (idxAbove.isValid())
			fsTree->setCurrentIndex(idxAbove);
	}
	else
		selectCurrentViewDir();
}

void Phototonic::createSubDirectory()
{
	QModelIndexList selectedDirs = fsTree->selectionModel()->selectedRows();
	QFileInfo dirInfo = QFileInfo(fsTree->fsModel->filePath(selectedDirs[0]));

	bool ok;
	QString newDirName = QInputDialog::getText(this, tr("New Sub folder"), 
							tr("New folder name:"), QLineEdit::Normal, "", &ok);

	if (!ok)
	{
		selectCurrentViewDir();
		return;
	}

	if(newDirName.isEmpty())
	{
		QMessageBox msgBox;
		msgBox.critical(this, tr("Error"), tr("Invalid name entered."));
		selectCurrentViewDir();
		return;
	}

	QDir dir(dirInfo.absoluteFilePath());
	ok = dir.mkdir(dirInfo.absoluteFilePath() + QDir::separator() + newDirName);

	if (!ok)
	{
		QMessageBox msgBox;
		msgBox.critical(this, tr("Error"), tr("Failed to create new folder."));
		selectCurrentViewDir();
		return;
	}

	setStatus(tr("Created \"%1\"").arg(newDirName));
	fsTree->expand(selectedDirs[0]);
}

QString Phototonic::getSelectedPath()
{
	QModelIndexList selectedDirs = fsTree->selectionModel()->selectedRows();
	if (selectedDirs.size() && selectedDirs[0].isValid())
	{
		QFileInfo dirInfo = QFileInfo(fsTree->fsModel->filePath(selectedDirs[0]));
		return dirInfo.absoluteFilePath();
	}
	else
		return "";
}

void Phototonic::wheelEvent(QWheelEvent *event)
{
	if (GData::layoutMode == imageViewIdx
			|| QApplication::focusWidget() == imageView->scrlArea)
	{	
		if (event->modifiers() == Qt::ControlModifier)
		{
			if (event->delta() < 0)
				zoomOut();				
			else
				zoomIn();
		}
		else if (nextImageAction->isEnabled())
		{
			if (event->delta() < 0)
				loadNextImage();
			else
				loadPrevImage();
		}
	}
}

void Phototonic::showNewImageWarning(QWidget *parent)
{
	QMessageBox msgBox;
	msgBox.warning(parent, tr("Warning"), tr("Cannot perform action with temporary image."));
}

bool Phototonic::removeDirOp(QString dirToDelete)
{
	bool ok = true;
	QDir dir(dirToDelete);

	Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden |
					QDir::AllDirs | QDir::Files, QDir::DirsFirst))
	{
		if (info.isDir())
			ok = removeDirOp(info.absoluteFilePath());
		else 
			ok = QFile::remove(info.absoluteFilePath());

		if (!ok)
			return ok;
	}
	ok = dir.rmdir(dirToDelete);

	return ok;
}

void Phototonic::cleanupCropDialog()
{
	setInterfaceEnabled(true);
}

void Phototonic::cleanupScaleDialog()
{
	delete resizeDialog;
	resizeDialog = 0;
	setInterfaceEnabled(true);
}

void Phototonic::cleanupColorsDialog()
{
	GData::colorsActive = false;
	setInterfaceEnabled(true);
}

void Phototonic::setInterfaceEnabled(bool enable)
{
	// actions 
	colorsAct->setEnabled(enable);
	renameAction->setEnabled(enable);
	cropAct->setEnabled(enable);
	resizeAct->setEnabled(enable);
	closeImageAct->setEnabled(enable);
	nextImageAction->setEnabled(enable);
	prevImageAction->setEnabled(enable);
	firstImageAction->setEnabled(enable);
	lastImageAction->setEnabled(enable);
	randomImageAction->setEnabled(enable);
	slideShowAction->setEnabled(enable);
	copyToAction->setEnabled(enable);
	moveToAction->setEnabled(enable);
	deleteAction->setEnabled(enable);
	settingsAction->setEnabled(enable);
	openAction->setEnabled(enable);

	// other
	thumbView->setEnabled(enable);
	fsTree->setEnabled(enable);
	bookmarks->setEnabled(enable);
	menuBar()->setEnabled(enable);
	editToolBar->setEnabled(enable);
	goToolBar->setEnabled(enable);
	viewToolBar->setEnabled(enable);
	interfaceDisabled = !enable;

	if (enable) {
		if (isFullScreen())
			imageView->setCursorHiding(true);
	} else {
		imageView->setCursorHiding(false);
	}
}

void Phototonic::addNewBookmark()
{
	addBookmark(getSelectedPath());
}

void Phototonic::addBookmark(QString path)
{
	GData::bookmarkPaths.insert(path);
	bookmarks->reloadBookmarks();
}

void Phototonic::findDuplicateImages()
{
	refreshThumbs(true);	
}

