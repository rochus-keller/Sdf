/*
* Copyright 2019 Rochus Keller <mailto:me@rochus-keller.ch>
*
* This file is part of the Sdf (Standard Delay Format) parser library.
*
* The following is the license that applies to this copy of the
* library. For a license to use the library under conditions
* other than those described here, please email to me@rochus-keller.ch.
*
* GNU General Public License Usage
* This file may be used under the terms of the GNU General Public
* License (GPL) versions 2.0 or 3.0 as published by the Free Software
* Foundation and appearing in the file LICENSE.GPL included in
* the packaging of this file. Please review the following information
* to ensure GNU General Public Licensing requirements will be met:
* http://www.fsf.org/licensing/licenses/info/GPLv2.html and
* http://www.gnu.org/copyleft/gpl.html.
*/

#include <QCoreApplication>
#include <QFile>
#include <QtDebug>
#include <QFileInfo>
#include <QDir>
#include <QElapsedTimer>
#include <QThread>
#include "SdfErrors.h"
#include "SdfParser.h"

static QStringList collectFiles( const QDir& dir )
{
    QStringList res;
    QStringList files = dir.entryList( QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name );

    foreach( const QString& f, files )
        res += collectFiles( QDir( dir.absoluteFilePath(f) ) );

    files = dir.entryList( QStringList() << QString("*.sdf"), QDir::Files, QDir::Name );
    foreach( const QString& f, files )
    {
        res.append( dir.absoluteFilePath(f) );
    }
    return res;
}

static void lexerTest( const QString& path )
{
    qDebug() << "***** reading" << path;
    QFile in( path );
    if( !in.open(QIODevice::ReadOnly) )
    {
        qDebug() << "**** cannot open file" << path;
        return ;
    }
    Sdf::Lexer lex;
    lex.setIgnoreComments(false);
    lex.setPackComments(true);
    lex.setStream( &in, path );

    Sdf::Token t = lex.nextToken();
    while( true )
    {
        if( t.isEof() )
        {
            qDebug() << "OK";
            break;
        }else if( !t.isValid() )
        {
            qDebug() << "FAILED" << t.d_lineNr << t.d_colNr << QString::fromLatin1(t.d_val);
            break;
        }else
            qDebug() << t.getName() << t.d_lineNr << t.d_colNr << QString::fromLatin1(t.d_val);
        t = lex.nextToken();
    }
}

static void dumpTree( Sdf::SynTree* node, int level = 0)
{
    QByteArray str;
    if( node->d_tok.d_type == Sdf::Tok_Invalid )
        level--;
    else if( node->d_tok.d_type < Sdf::SynTree::R_First )
    {
        if( Sdf::tokenTypeIsKeyword( node->d_tok.d_type ) )
            str = Sdf::tokenTypeString(node->d_tok.d_type);
        else if( node->d_tok.d_type > Sdf::TT_Specials )
            str = QByteArray("\"") + node->d_tok.d_val + QByteArray("\"");
        else
            str = QByteArray("\"") + node->d_tok.getString() + QByteArray("\"");

    }else
        str = Sdf::SynTree::rToStr( node->d_tok.d_type );
    if( !str.isEmpty() )
    {
        str += QByteArray("\t") + QFileInfo(node->d_tok.d_sourcePath).baseName().toUtf8() +
                ":" + QByteArray::number(node->d_tok.d_lineNr) +
                ":" + QByteArray::number(node->d_tok.d_colNr);
        QByteArray ws;
        for( int i = 0; i < level; i++ )
            ws += "|  ";
        str = ws + str;
        qDebug() << str.data();
    }
    foreach( Sdf::SynTree* sub, node->d_children )
        dumpTree( sub, level + 1 );
}

static void parserTest( const QString& path )
{
    qDebug() << "***** reading" << path;
    QFile in( path );
    if( !in.open(QIODevice::ReadOnly) )
    {
        qDebug() << "**** cannot open file" << path;
        return;
    }
    Sdf::Errors e;
    e.setReportToConsole(true);
    Sdf::Lexer lex;
    lex.setIgnoreComments(false);
    lex.setPackComments(true);
    lex.setStream( &in, path );
    lex.setErrors(&e);
    Sdf::Parser p(&lex,&e);
    p.RunParser();
    if( e.getErrCount() == 0 )
        qDebug() << "OK";
    else
        qDebug() << "FAILED";
    if(e.getErrCount() == 0)
        dumpTree( &p.d_root );
}

static void crossRefTest( const QStringList& paths )
{
    //Sdf::CodeModel m;
    //m.parseFiles(paths);
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QString dirOrFilePath;
    bool isProject = false;
    const QStringList args = QCoreApplication::arguments();
    for( int i = 1; i < args.size(); i++ ) // arg 0 enthaelt Anwendungspfad
    {
        if( args[i].startsWith( "-p" ) )
            isProject = true;
        else if( !args[ i ].startsWith( '-' ) )
        {
            dirOrFilePath = args[ i ];
        }else
        {
            qDebug() << "invalid command line parameter" << args[i];
            return -1;
        }
    }
    if( dirOrFilePath.isEmpty() )
    {
        qCritical() << "expecting a directory or file path";
        return -1;
    }

    QStringList files;
    QFileInfo info(dirOrFilePath);
    if( info.isDir() )
        files = collectFiles( info.absoluteFilePath() );
    else
        files << dirOrFilePath;

    foreach( const QString& path, files )
    {
        //lexerTest( path );
        parserTest( path );
    }
    crossRefTest(files);

    return 0 ;
}
