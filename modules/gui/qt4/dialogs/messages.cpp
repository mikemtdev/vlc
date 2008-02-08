/*****************************************************************************
 * Messages.cpp : Information about an item
 ****************************************************************************
 * Copyright (C) 2006-2007 the VideoLAN team
 * $Id$
 *
 * Authors: Jean-Baptiste Kempf <jb (at) videolan.org>
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "dialogs/messages.hpp"
#include "dialogs_provider.hpp"

#include <QSpacerItem>
#include <QSpinBox>
#include <QLabel>
#include <QTextEdit>
#include <QTextCursor>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QTabWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>

MessagesDialog *MessagesDialog::instance = NULL;

MessagesDialog::MessagesDialog( intf_thread_t *_p_intf) :  QVLCFrame( _p_intf )
{
    setWindowTitle( qtr( "Messages" ) );
    resize( 600, 450 );

    /* General widgets */
    QGridLayout *mainLayout = new QGridLayout( this );
    QTabWidget  *mainTab = new QTabWidget( this );
    mainTab->setTabPosition( QTabWidget::North );

    QPushButton *closeButton = new QPushButton( qtr( "&Close" ) );
    closeButton->setDefault( true );
    BUTTONACT( closeButton, close() );

    mainLayout->addWidget( mainTab, 0, 0, 1, 0 );
    mainLayout->addWidget( closeButton, 1, 5 );


    /* Messages */
    QWidget     *msgWidget = new QWidget;
    QGridLayout *msgLayout = new QGridLayout( msgWidget );

    QPushButton *clearButton = new QPushButton( qtr( "&Clear" ) );
    QPushButton *saveLogButton = new QPushButton( qtr( "&Save as..." ) );

    verbosityBox = new QSpinBox();
    verbosityBox->setRange( 0, 2 );
    verbosityBox->setValue( config_GetInt( p_intf, "verbose" ) );
    verbosityBox->setWrapping( true );
    verbosityBox->setMaximumWidth( 50 );

    QLabel *verbosityLabel = new QLabel( qtr( "Verbosity Level" ) );

    messages = new QTextEdit();
    messages->setReadOnly( true );
    messages->setGeometry( 0, 0, 440, 600 );
    messages->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

    msgLayout->addWidget( messages, 0, 0, 1, 0 );
    msgLayout->addWidget( verbosityLabel, 1, 0, 1, 1 );
    msgLayout->addWidget( verbosityBox, 1, 1 );
    msgLayout->addItem( new QSpacerItem( 20, 20, QSizePolicy::Expanding ), 1, 2 );
    msgLayout->addWidget( saveLogButton, 1, 3 );
    msgLayout->addWidget( clearButton, 1, 4 );

    BUTTONACT( clearButton, clear() );
    BUTTONACT( saveLogButton, save() );
    ON_TIMEOUT( updateLog() );

    mainTab->addTab( msgWidget, qtr( "Messages" ) );

    /* Module tree */
    QWidget     *treeWidget = new QWidget;
    QGridLayout *treeLayout = new QGridLayout( treeWidget );

    modulesTree = new QTreeWidget();
    modulesTree->setGeometry( 0, 0, 440, 600 );

    QPushButton *updateButton = new QPushButton( qtr( "&Update" ) );

    treeLayout->addWidget( modulesTree, 0, 0, 1, 0 );
    treeLayout->addWidget( updateButton, 1, 6 );

    BUTTONACT( updateButton, updateTree() );

    mainTab->addTab( treeWidget, qtr( "Modules tree" ) );


    readSettings( "Messages" );
}

void MessagesDialog::updateLog()
{
    msg_subscription_t *p_sub = p_intf->p_sys->p_sub;
    int i_start;

    vlc_mutex_lock( p_sub->p_lock );
    int i_stop = *p_sub->pi_stop;
    vlc_mutex_unlock( p_sub->p_lock );

    if( p_sub->i_start != i_stop )
    {
        messages->textCursor().movePosition( QTextCursor::End );

        for( i_start = p_sub->i_start;
                i_start != i_stop;
                i_start = (i_start+1) % VLC_MSG_QSIZE )
        {
            if( p_sub->p_msg[i_start].i_type == VLC_MSG_INFO ||
                p_sub->p_msg[i_start].i_type == VLC_MSG_ERR ||
                p_sub->p_msg[i_start].i_type == VLC_MSG_WARN &&
                    verbosityBox->value() >= 1 ||
                p_sub->p_msg[i_start].i_type == VLC_MSG_DBG &&
                    verbosityBox->value() >= 2 )
            {
                messages->setFontItalic( true );
                messages->setTextColor( "darkBlue" );
                messages->insertPlainText( qfu( p_sub->p_msg[i_start].psz_module ) );
            }
            else
                continue;

            switch( p_sub->p_msg[i_start].i_type )
            {
                case VLC_MSG_INFO:
                    messages->setTextColor( "blue" );
                    messages->insertPlainText( " info: " );
                    break;
                case VLC_MSG_ERR:
                    messages->setTextColor( "red" );
                    messages->insertPlainText( " error: " );
                    break;
                case VLC_MSG_WARN:
                    messages->setTextColor( "green" );
                    messages->insertPlainText( " warning: " );
                    break;
                case VLC_MSG_DBG:
                default:
                    messages->setTextColor( "grey" );
                    messages->insertPlainText( " debug: " );
                    break;
            }

            /* Add message Regular black Font */
            messages->setFontItalic( false );
            messages->setTextColor( "black" );
            messages->insertPlainText( qfu(p_sub->p_msg[i_start].psz_msg) );
            messages->insertPlainText( "\n" );
        }
        messages->ensureCursorVisible();

        vlc_mutex_lock( p_sub->p_lock );
        p_sub->i_start = i_start;
        vlc_mutex_unlock( p_sub->p_lock );
    }
}

void MessagesDialog::buildTree( QTreeWidgetItem *parentItem, vlc_object_t *p_obj )
{
    vlc_object_yield( p_obj );
    QTreeWidgetItem *item;

    if( parentItem )
        item = new QTreeWidgetItem( parentItem );
    else
        item = new QTreeWidgetItem( modulesTree );

    if( p_obj->psz_object_name )
        item->setText( 0, qfu( p_obj->psz_object_type ) + " \"" + qfu( p_obj->psz_object_name ) + "\" (" + QString::number(p_obj->i_object_id) + ")\n" );
    else
        item->setText( 0, qfu( p_obj->psz_object_type ) + " (" + QString::number(p_obj->i_object_id) + ")\n" );

    for( int i=0; i < p_obj->i_children; i++ )
    {
        buildTree( item, p_obj->pp_children[i]);
    }

    vlc_object_release( p_obj );
}

void MessagesDialog::updateTree()
{
    modulesTree->clear();

    buildTree( NULL, VLC_OBJECT( p_intf->p_libvlc ) );
}

void MessagesDialog::close()
{
    hide();
}

void MessagesDialog::clear()
{
    messages->clear();
}

bool MessagesDialog::save()
{
    QString saveLogFileName = QFileDialog::getSaveFileName(
            this, qtr( "Choose a filename to save the logs under..." ),
            qfu( p_intf->p_libvlc->psz_homedir ),
            qtr( "Texts / Logs (*.log *.txt);; All (*.*) ") );

    if( !saveLogFileName.isNull() )
    {
        QFile file( saveLogFileName );
        if ( !file.open( QFile::WriteOnly | QFile::Text ) ) {
            QMessageBox::warning( this, qtr( "Application" ),
                    qtr( "Cannot write file %1:\n%2." )
                    .arg( saveLogFileName )
                    .arg( file.errorString() ) );
            return false;
        }

        QTextStream out( &file );
        out << messages->toPlainText() << "\n";

        return true;
    }
    return false;
}
