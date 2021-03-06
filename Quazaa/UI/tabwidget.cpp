/*
* Copyright (C) 2008-2013 J-P Nurmi <jpnurmi@gmail.com>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#include "tabwidget.h"
#include "tabwidget_p.h"
#include "sharedtimer.h"
#include <QContextMenuEvent>
#include <QStackedWidget>
#include <QShortcut>

CTabBar::CTabBar(QWidget* parent) : QTabBar(parent)
{
    prevShortcut = new QShortcut(this);
    connect(prevShortcut, SIGNAL(activated()), parent, SLOT(moveToPrevTab()));

    nextShortcut = new QShortcut(this);
    connect(nextShortcut, SIGNAL(activated()), parent, SLOT(moveToNextTab()));

    prevUnreadShortcut = new QShortcut(this);
    connect(prevUnreadShortcut, SIGNAL(activated()), parent, SLOT(moveToPrevUnreadTab()));

    nextUnreadShortcut = new QShortcut(this);
    connect(nextUnreadShortcut, SIGNAL(activated()), parent, SLOT(moveToNextUnreadTab()));

    int i = addTab("");
    setTabIcon(i, QIcon(":/Resource/Generic/Add.png"));

    setDocumentMode(true);
    setSelectionBehaviorOnRemove(SelectLeftTab);
}

QKeySequence CTabBar::navigationShortcut(Navigation navigation) const
{
    switch (navigation) {
        case Next:           return nextShortcut->key();
        case Previous:       return prevShortcut->key();
        case NextUnread:     return nextUnreadShortcut->key();
        case PreviousUnread: return prevUnreadShortcut->key();
        default:             return QKeySequence();
    }
}

void CTabBar::setNavigationShortcut(Navigation navigation, const QKeySequence& shortcut)
{
    switch (navigation) {
        case Next:           return nextShortcut->setKey(shortcut);
        case Previous:       return prevShortcut->setKey(shortcut);
        case NextUnread:     return nextUnreadShortcut->setKey(shortcut);
        case PreviousUnread: return prevUnreadShortcut->setKey(shortcut);
    }
}

QSize CTabBar::minimumSizeHint() const
{
    return testAttribute(Qt::WA_WState_Hidden) ? QSize() : QTabBar::minimumSizeHint();
}

void CTabBar::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::StyleChange) {
        Qt::TextElideMode mode = elideMode();
        QTabBar::changeEvent(event);
        if (mode != elideMode())
            setElideMode(mode);
        return;
    }
    QTabBar::changeEvent(event);
}

void CTabBar::contextMenuEvent(QContextMenuEvent* event)
{
    int index = tabAt(event->pos());
    if (index != -1)
        emit menuRequested(index, event->globalPos());
}

void CTabBar::wheelEvent(QWheelEvent* event)
{
    if (event->delta() > 0)
        QMetaObject::invokeMethod(parent(), "moveToPrevTab");
    else
        QMetaObject::invokeMethod(parent(), "moveToNextTab");
    QWidget::wheelEvent(event);
}

CTabWidget::CTabWidget(QWidget* parent) : QTabWidget(parent)
{
    setTabBar(new CTabBar(this));
    setElideMode(Qt::ElideMiddle);
    d.previous = -1;
    d.updatingColors = false;

    d.colors[Active] = palette().color(QPalette::WindowText);
    d.colors[Inactive] = palette().color(QPalette::Disabled, QPalette::Highlight);
    d.colors[Alert] = palette().color(QPalette::Highlight);
    d.colors[Highlight] = palette().color(QPalette::Highlight);

    connect(this, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
    connect(tabBar(), SIGNAL(menuRequested(int, QPoint)), this, SIGNAL(tabMenuRequested(int, QPoint)));
}

QTabBar* CTabWidget::tabBar() const
{
    return QTabWidget::tabBar();
}

QSize CTabWidget::sizeHint() const
{
    if (tabBar()->testAttribute(Qt::WA_WState_Hidden)) {
        QStackedWidget* stack = findChild<QStackedWidget*>("qt_tabwidget_stackedwidget");
        return stack->sizeHint();
    }
    return QTabWidget::sizeHint();
}

QColor CTabWidget::tabTextColor(CTabWidget::TabRole role) const
{
    return d.colors.value(role);
}

void CTabWidget::setTabTextColor(CTabWidget::TabRole role, const QColor& color)
{
    d.colors[role] = color;
    if (!d.updatingColors) {
        d.updatingColors = true;
        QMetaObject::invokeMethod(this, "updateTabColors", Qt::QueuedConnection);
    }
}

bool CTabWidget::isTabInactive(int index)
{
    if (index == -1)
        return !d.inactiveIndexes.isEmpty();
    return d.inactiveIndexes.contains(index);
}

void CTabWidget::setTabInactive(int index, bool inactive)
{
    if (!inactive)
        d.inactiveIndexes.removeAll(index);
    else if (!d.inactiveIndexes.contains(index))
        d.inactiveIndexes.append(index);
    colorizeTab(index);
}

bool CTabWidget::hasTabAlert(int index)
{
    if (index == -1)
        return !d.alertIndexes.isEmpty();
    return d.alertIndexes.contains(index);
}

void CTabWidget::setTabAlert(int index, bool alert)
{
    if (!alert) {
        int count = d.alertIndexes.removeAll(index);
        if (count > 0 && d.alertIndexes.isEmpty()) {
            emit alertStatusChanged(false);
            SharedTimer::instance()->unregisterReceiver(this, "alertTimeout");
        }
    } else if (!d.alertIndexes.contains(index)) {
        if (d.alertIndexes.isEmpty())
            SharedTimer::instance()->registerReceiver(this, "alertTimeout");
        d.alertIndexes.append(index);
        if (d.alertIndexes.count() == 1)
            emit alertStatusChanged(true);
    }
    colorizeTab(index);
}

bool CTabWidget::hasTabHighlight(int index) const
{
    if (index == -1)
        return !d.highlightIndexes.isEmpty();
    return d.highlightIndexes.contains(index);
}

void CTabWidget::setTabHighlight(int index, bool highlight)
{
    if (!highlight) {
        int count = d.highlightIndexes.removeAll(index);
        if (count > 0 && d.highlightIndexes.isEmpty())
            emit highlightStatusChanged(false);
    } else if (!d.highlightIndexes.contains(index)) {
        d.highlightIndexes.append(index);
        if (d.highlightIndexes.count() == 1)
            emit highlightStatusChanged(true);
    }
    colorizeTab(index);
}

void CTabWidget::moveToNextTab()
{
    int index = currentIndex();
    if (++index >= count() - 1)
        index = 0;
    setCurrentIndex(index);
}

void CTabWidget::moveToPrevTab()
{
    int index = currentIndex();
    if (--index < 0)
        index = count() - 2;
    setCurrentIndex(index);
}

void CTabWidget::moveToNextUnreadTab()
{
    int index = currentIndex();
    while (++index < count()) {
        if (hasTabAlert(index) || hasTabHighlight(index)) {
            setCurrentIndex(index);
            break;
        }
    }
}

void CTabWidget::moveToPrevUnreadTab()
{
    int index = currentIndex();
    while (index-- > 0) {
        if (hasTabAlert(index) || hasTabHighlight(index)) {
            setCurrentIndex(index);
            break;
        }
    }
}

static void shiftIndexesFrom(QList<int>& indexes, int from, int delta)
{
    QMutableListIterator<int> it(indexes);
    while (it.hasNext()) {
        if (it.next() >= from)
            it.value() += delta;
    }
}

void CTabWidget::tabInserted(int index)
{
    shiftIndexesFrom(d.inactiveIndexes, index, 1);
    shiftIndexesFrom(d.alertIndexes, index, 1);
    shiftIndexesFrom(d.highlightIndexes, index, 1);
    colorizeTab(index);
}

void CTabWidget::tabRemoved(int index)
{
    d.inactiveIndexes.removeAll(index);
    d.alertIndexes.removeAll(index);
    d.highlightIndexes.removeAll(index);
    shiftIndexesFrom(d.inactiveIndexes, index, -1);
    shiftIndexesFrom(d.alertIndexes, index, -1);
    shiftIndexesFrom(d.highlightIndexes, index, -1);
}

void CTabWidget::tabChanged(int index)
{
    if (index == count() - 1) {
        emit newTabRequested();
        if (d.previous != -1)
            setCurrentIndex(d.previous);
    } else {
        d.previous = index;
    }
}

void CTabWidget::alertTimeout()
{
    if (d.currentAlertColor == d.colors.value(Alert))
        d.currentAlertColor = d.colors.value(Active);
    else
        d.currentAlertColor = d.colors.value(Alert);

    foreach (int index, d.alertIndexes)
        colorizeTab(index);
}

void CTabWidget::colorizeTab(int index)
{
    if (isTabInactive(index))
        tabBar()->setTabTextColor(index, d.colors.value(Inactive));
    else if (hasTabAlert(index))
        tabBar()->setTabTextColor(index, d.currentAlertColor);
    else if (hasTabHighlight(index))
        tabBar()->setTabTextColor(index, d.colors.value(Highlight));
    else
        tabBar()->setTabTextColor(index, d.colors.value(Active));
}

void CTabWidget::updateTabColors()
{
    for (int i = 0; i < count(); ++i)
        colorizeTab(i);
    d.updatingColors = false;
}
