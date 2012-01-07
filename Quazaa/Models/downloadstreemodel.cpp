/*
** $Id$
**
** Copyright © Quazaa Development Team, 2009-2011.
** This file is part of QUAZAA (quazaa.sourceforge.net)
**
** Quazaa is free software; this file may be used under the terms of the GNU
** General Public License version 3.0 or later as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Quazaa is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
**
** Please review the following information to ensure the GNU General Public
** License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** You should have received a copy of the GNU General Public License version
** 3.0 along with Quazaa; if not, write to the Free Software Foundation,
** Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "downloadstreemodel.h"

#ifdef _DEBUG
#include "debug_new.h"
#endif

CDownloadsTreeModel::CDownloadsTreeModel(QObject *parent) :
    QAbstractItemModel(parent)
{
	rootItem = new CDownloadsItemBase(this);
}

CDownloadsTreeModel::~CDownloadsTreeModel()
{
	delete rootItem;
}

QVariant CDownloadsTreeModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if (role != Qt::DisplayRole)
		return QVariant();

	CDownloadsItemBase *item = static_cast<CDownloadsItemBase*>(index.internalPointer());

	return item->data(index.column());
}

Qt::ItemFlags CDownloadsTreeModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return 0;

	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant CDownloadsTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
	{
		switch(section)
		{
		case CDownloadsTreeModel::NAME:
				return tr("Downloaded file");
			break;
		case CDownloadsTreeModel::SIZE:
				return tr("Size");
			break;
		case CDownloadsTreeModel::PROGRESS:
				return tr("Progress");
			break;
		case CDownloadsTreeModel::BANDWIDTH:
				return tr("Bandwidth");
			break;
		case CDownloadsTreeModel::STATUS:
				return tr("Status");
			break;
		case CDownloadsTreeModel::PRIORITY:
				return tr("Priority");
			break;
		case CDownloadsTreeModel::CLIENT:
				return tr("Client");
			break;
		case CDownloadsTreeModel::COMPLETED:
				return tr("Completed");
			break;
		case CDownloadsTreeModel::COUNTRY:
				return tr("Country");
			break;
		default:
				return QVariant();
		}
	}

	return QVariant();
}

QModelIndex CDownloadsTreeModel::index(int row, int column, const QModelIndex &parent) const
{
	if (!hasIndex(row, column, parent))
		return QModelIndex();

	CDownloadsItemBase *parentItem;

	if (!parent.isValid())
		parentItem = rootItem;
	else
		parentItem = static_cast<CDownloadsItemBase*>(parent.internalPointer());

	CDownloadsItemBase *childItem = parentItem->child(row);
	if (childItem)
		return createIndex(row, column, childItem);
	else
		return QModelIndex();
}

QModelIndex CDownloadsTreeModel::parent(const QModelIndex &index) const
{
	if (!index.isValid())
		return QModelIndex();

	CDownloadsItemBase *childItem = static_cast<CDownloadsItemBase*>(index.internalPointer());
	CDownloadsItemBase *parentItem = childItem->parent();

	if (parentItem == rootItem)
		return QModelIndex();

	return createIndex(parentItem->row(), 0, parentItem);
}

int CDownloadsTreeModel::rowCount(const QModelIndex &parent) const
{
	CDownloadsItemBase *parentItem;
	if (parent.column() > 0)
		return 0;

	if (!parent.isValid())
		parentItem = rootItem;
	else
		parentItem = static_cast<CDownloadsItemBase*>(parent.internalPointer());

	return parentItem->childCount();
}

int CDownloadsTreeModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return _NO_OF_COLUMNS;
}



CDownloadsItemBase::CDownloadsItemBase(QObject *parent)
	: QObject(parent),
	  parentItem(0)
{
}

CDownloadsItemBase::~CDownloadsItemBase()
{
	qDeleteAll(childItems);
}

void CDownloadsItemBase::appendChild(CDownloadsItemBase *child)
{
	childItems.append(child);
}

CDownloadsItemBase *CDownloadsItemBase::child(int row)
{
	return childItems.value(row);
}

int CDownloadsItemBase::childCount() const
{
	return childItems.count();
}

int CDownloadsItemBase::columnCount() const
{
	return CDownloadsTreeModel::_NO_OF_COLUMNS;
}

QVariant CDownloadsItemBase::data(int column) const
{
	Q_UNUSED(column);
	return QVariant();
}

int CDownloadsItemBase::row() const
{
	if( parentItem )
		return parentItem->childItems.indexOf(const_cast<CDownloadsItemBase*>(this));
	return 0;
}

CDownloadsItemBase *CDownloadsItemBase::parent()
{
	return parentItem;
}

CDownloadItem::CDownloadItem(CDownload *download, CDownloadsItemBase *parent, QObject *parentQObject)
	: CDownloadsItemBase(parentQObject),
	  m_pDownload(download)
{
	parentItem = parent;

	// TODO: import values from CDownload
	m_sName = "";
	m_nSize = 0;
	m_nProgress = 0;
	m_nBandwidth = 0;
	m_nStatus = 0;
	m_nPriority = 0;
	m_nCompleted = 0;
}

CDownloadItem::~CDownloadItem()
{
}

void CDownloadItem::appendChild(CDownloadsItemBase *child)
{
	Q_ASSERT_X(qobject_cast<CDownloadSourceItem*>(child) != 0, "CDownloadItem::appendChild()", "child can only be CDownloadSourceItem!");

	CDownloadsItemBase::appendChild(child);
}

QVariant CDownloadItem::data(int column) const
{
	switch(column)
	{
	case CDownloadsTreeModel::NAME:
			return m_sName;
		break;
	case CDownloadsTreeModel::SIZE:
			return m_nSize;
		break;
	case CDownloadsTreeModel::PROGRESS:
			return m_nProgress;
		break;
	case CDownloadsTreeModel::BANDWIDTH:
			return m_nBandwidth;
		break;
	case CDownloadsTreeModel::STATUS:
			return m_nStatus;
		break;
	case CDownloadsTreeModel::PRIORITY:
			return m_nPriority;
		break;
	case CDownloadsTreeModel::CLIENT:
			return QVariant();
		break;
	case CDownloadsTreeModel::COMPLETED:
			return m_nCompleted;
		break;
	case CDownloadsTreeModel::COUNTRY:
			return QVariant();
		break;
	default:
			return QVariant();
		break;

	}
}

CDownloadSourceItem::CDownloadSourceItem(CDownloadSource *downloadSource, CDownloadsItemBase *parent, QObject *parentQObject)
	: CDownloadsItemBase(parentQObject),
	  m_pDownloadSource(downloadSource)
{
	parentItem = parent;

	// TODO: import values from downloadSource
	m_sAddress = "";
	m_nSize = 0;
	m_nProgress = 0;
	m_nBandwidth = 0;
	m_nStatus = 0;
	m_sClient = "";
	m_nDownloaded = 0;
	m_sCountry = "";
}

CDownloadSourceItem::~CDownloadSourceItem()
{
}

void CDownloadSourceItem::appendChild(CDownloadsItemBase *child)
{
	Q_ASSERT_X(false, "CDownloadSourceItem::appendChild()", "CDownloadSourceItem cannot have children!");
	delete child; // good?
}

QVariant CDownloadSourceItem::data(int column) const
{
	switch(column)
	{
	case CDownloadsTreeModel::NAME:
			return m_sAddress;
		break;
	case CDownloadsTreeModel::SIZE:
			return m_nSize;
		break;
	case CDownloadsTreeModel::PROGRESS:
			return QVariant();
		break;
	case CDownloadsTreeModel::BANDWIDTH:
			return m_nBandwidth;
		break;
	case CDownloadsTreeModel::STATUS:
			return m_nStatus;
		break;
	case CDownloadsTreeModel::PRIORITY:
			return QVariant();
		break;
	case CDownloadsTreeModel::CLIENT:
			return m_sClient;
		break;
	case CDownloadsTreeModel::COMPLETED:
			return m_nDownloaded;
		break;
	case CDownloadsTreeModel::COUNTRY:
			return m_sCountry;
		break;
	default:
			return QVariant();
		break;

	}
}
