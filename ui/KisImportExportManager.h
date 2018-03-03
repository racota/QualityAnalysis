﻿/*
 * Copyright (C) 2016 Boudewijn Rempt <boud@valdyas.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef KIS_IMPORT_EXPORT_MANAGER_H
#define KIS_IMPORT_EXPORT_MANAGER_H

#include <QObject>
#include <QMap>
#include <QByteArray>
#include <QUrl>

#include "KisImportExportFilter.h"

#include "kritaui_export.h"

class KisDocument;
class KoProgressUpdater;

template <class T>
class QFuture;

/**
 *  @brief The class managing all the filters.
 *
 *  This class manages all filters for a %Calligra application. Normally
 *  you will not have to use it, since KisMainWindow takes care of loading
 *  and saving documents.
 *
 *  @ref KisFilter
 *
 *  @author Kalle Dalheimer <kalle@kde.org>
 *  @author Torben Weis <weis@kde.org>
 *  @author Werner Trobin <trobin@kde.org>
 */
class KRITAUI_EXPORT KisImportExportManager : public QObject
{
    Q_OBJECT
public:
    /**
     * This enum is used to distinguish the import/export cases
     */
    enum Direction { Import = 1,  Export = 2 };

    /**
     * Create a filter manager for a document
     */
    explicit KisImportExportManager(KisDocument *document);

public:

    ~KisImportExportManager() override;

    /**
     * Imports the specified document and returns the resultant filename
     * (most likely some file in /tmp).
     * @p path can be either a URL or a filename.
     * @p documentMimeType gives importDocument a hint about what type
     * the document may be. It can be left empty.
     *
     * @return  status signals the success/error of the conversion.
     * If the QString which is returned isEmpty() and the status is OK,
     * then we imported the file directly into the document.
     */
    KisImportExportFilter::ConversionStatus importDocument(const QString &location, const QString &mimeType);

    /**
     * @brief Exports the given file/document to the specified URL/mimetype.
     *
     * If @p mimeType is empty, then the closest matching Calligra part is searched
     * and when the method returns @p mimeType contains this mimetype.
     * Oh, well, export is a C++ keyword ;)
     */
    KisImportExportFilter::ConversionStatus exportDocument(const QString &location, const QString& realLocation, const QByteArray &mimeType, bool showWarnings = true, KisPropertiesConfigurationSP exportConfiguration = 0);

    QFuture<KisImportExportFilter::ConversionStatus> exportDocumentAsyc(const QString &location, const QString& realLocation, const QByteArray &mimeType, KisImportExportFilter::ConversionStatus &status, bool showWarnings = true, KisPropertiesConfigurationSP exportConfiguration = 0);

    ///@name Static API
    //@{
    /**
     * Suitable for passing to KoFileDialog::setMimeTypeFilters. The default mime
     * gets set by the "users" of this method, as we do not have enough
     * information here.
     * Optionally, @p extraNativeMimeTypes are added after the native mimetype.
     */
    static QStringList mimeFilter(Direction direction);

    /**
     * @brief filterForMimeType loads the relevant import/export plugin and returns it. The caller
     * is responsible for deleting it!
     * @param mimetype the mimetype we want to import/export. If there's more than one plugin, the one
     * with the highest weight as defined in the json description will be taken
     * @param direction import or export
     * @return a pointer to the filter plugin or 0 if none could be found
     */
    static KisImportExportFilter *filterForMimeType(const QString &mimetype, Direction direction);

    /**
     * Get if the filter manager is batch mode (true)
     * or in interactive mode (true)
     */
    bool batchMode(void) const;

    void setUpdater(KoUpdaterPtr updater);

    static QString askForAudioFileName(const QString &defaultDir, QWidget *parent);


private:

    struct ConversionResult;
    ConversionResult convert(Direction direction, const QString &location, const QString& realLocation, const QString &mimeType, bool showWarnings, KisPropertiesConfigurationSP exportConfiguration, bool isAsync);


    void fillStaticExportConfigurationProperties(KisPropertiesConfigurationSP exportConfiguration);
    bool askUserAboutExportConfiguration(QSharedPointer<KisImportExportFilter> filter, KisPropertiesConfigurationSP exportConfiguration, const QByteArray &from, const QByteArray &to, bool batchMode, const bool showWarnings, bool *alsoAsKra);

    KisImportExportFilter::ConversionStatus doImport(const QString &location, QSharedPointer<KisImportExportFilter> filter);

    KisImportExportFilter::ConversionStatus doExport(const QString &location, QSharedPointer<KisImportExportFilter> filter, KisPropertiesConfigurationSP exportConfiguration, bool alsoAsKra);
    KisImportExportFilter::ConversionStatus doExportImpl(const QString &location, QSharedPointer<KisImportExportFilter> filter, KisPropertiesConfigurationSP exportConfiguration);

    // Private API
    KisImportExportManager(const KisImportExportManager& rhs);
    KisImportExportManager &operator=(const KisImportExportManager& rhs);

    KisDocument *m_document;

    /// A static cache for the availability checks of filters
    static QStringList m_importMimeTypes;
    static QStringList m_exportMimeTypes;

    class Private;
    Private * const d;
};

#endif  // __KO_FILTER_MANAGER_H__
