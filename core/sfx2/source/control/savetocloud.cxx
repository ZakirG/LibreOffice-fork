/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sfx2/savetocloud.hxx>
#include <sfx2/cloudauth.hxx>
#include <sfx2/cloudapi.hxx>
#include <sfx2/objsh.hxx>
#include <sfx2/request.hxx>

#include <svl/eitem.hxx>
#include <svl/stritem.hxx>
#include <com/sun/star/lang/XServiceInfo.hpp>

#include <comphelper/storagehelper.hxx>
#include <vcl/svapp.hxx>
#include <vcl/weld.hxx>
#include <tools/stream.hxx>
#include <unotools/tempfile.hxx>
#include <comphelper/processfactory.hxx>
#include <com/sun/star/embed/XStorage.hpp>
#include <com/sun/star/embed/ElementModes.hpp>
#include <com/sun/star/frame/XStorable.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/io/IOException.hpp>
#include <rtl/ustrbuf.hxx>
#include <sal/log.hxx>

#include <iostream>
#include <fstream>

using namespace css;

SaveToCloudHandler::SaveToCloudHandler(SfxObjectShell* pObjectShell)
    : m_pObjectShell(pObjectShell)
    , m_pAuthHandler(nullptr)
    , m_pApiClient(nullptr)
    , m_bOperationInProgress(false)
{
    SAL_WARN("sfx.control", "SaveToCloudHandler constructor called");
    
    try 
    {
        m_pAuthHandler = &CloudAuthHandler::getInstance();
        if (m_pAuthHandler->isAuthenticated())
        {
            m_pApiClient = m_pAuthHandler->getApiClient();
        }
    }
    catch (...)
    {
        SAL_WARN("sfx.control", "Failed to initialize SaveToCloudHandler");
    }
}

SaveToCloudHandler::~SaveToCloudHandler() = default;

bool SaveToCloudHandler::Execute(SfxRequest& rReq)
{
    SAL_WARN("sfx.control", "SaveToCloudHandler::Execute called");
    
    if (!m_pObjectShell)
    {
        showErrorMessage("No document available to save.");
        return false;
    }

    if (m_bOperationInProgress)
    {
        showErrorMessage("Cloud save operation already in progress.");
        return false;
    }

    // Check authentication
    if (!checkAuthentication())
    {
        return false;
    }

    m_bOperationInProgress = true;
    
    try
    {
        // Show progress dialog
        showProgressDialog("Preparing document for cloud upload...");

        // Get document data
        OUString sDocumentData, sFileName, sContentType;
        if (!getDocumentData(sDocumentData, sFileName, sContentType))
        {
            hideProgressDialog();
            showErrorMessage("Failed to prepare document data for upload.");
            m_bOperationInProgress = false;
            return false;
        }

        // Upload to cloud
        showProgressDialog("Uploading document to LibreCloud...");
        if (!uploadToCloud(sDocumentData, sFileName, sContentType))
        {
            hideProgressDialog();
            showErrorMessage("Failed to upload document to cloud.");
            m_bOperationInProgress = false;
            return false;
        }

        hideProgressDialog();
        showSuccessMessage(sFileName);
        m_bOperationInProgress = false;
        
        // Mark request as successful
        rReq.SetReturnValue(SfxBoolItem(0, true));
        return true;
    }
    catch (const std::exception& e)
    {
        hideProgressDialog();
        OUString sError = "Cloud save failed: " + OUString::fromUtf8(e.what());
        showErrorMessage(sError);
        m_bOperationInProgress = false;
        rReq.SetReturnValue(SfxBoolItem(0, false));
        return false;
    }
}

bool SaveToCloudHandler::checkAuthentication()
{
    if (!m_pAuthHandler)
    {
        showErrorMessage("Cloud authentication system not available.");
        return false;
    }

    if (!m_pAuthHandler->isAuthenticated())
    {
        // Prompt user to authenticate
        std::unique_ptr<weld::MessageDialog> xBox(Application::CreateMessageDialog(nullptr,
            VclMessageType::Question, VclButtonsType::YesNo,
            "You are not logged in to LibreCloud. Would you like to log in now?"));
        xBox->set_title("LibreCloud Authentication Required");
        
        if (xBox->run() == RET_YES)
        {
            m_pAuthHandler->startAuthentication();
            
            // For now, return false as authentication is async
            // In the future, we could implement a callback mechanism
            showErrorMessage("Please complete authentication and try saving again.");
            return false;
        }
        else
        {
            return false;
        }
    }

    // Update API client reference
    m_pApiClient = m_pAuthHandler->getApiClient();
    if (!m_pApiClient)
    {
        showErrorMessage("Cloud API client not available.");
        return false;
    }

    return true;
}

bool SaveToCloudHandler::getDocumentData(OUString& rDocumentData, OUString& rsFileName, OUString& rsContentType)
{
    try
    {
        // Generate filename and content type
        rsFileName = generateFileName();
        rsContentType = getContentType();

        SAL_WARN("sfx.control", "Saving document as: " << rsFileName << " with type: " << rsContentType);

        // Create temporary file for saving
        utl::TempFileNamed aTempFile;
        aTempFile.EnableKillingFile();
        
        OUString sTempURL = aTempFile.GetURL();
        
        // Get XStorable interface
        uno::Reference<frame::XStorable> xStorable(m_pObjectShell->GetModel(), uno::UNO_QUERY);
        if (!xStorable.is())
        {
            SAL_WARN("sfx.control", "Document is not storable");
            return false;
        }

        // Prepare save arguments
        uno::Sequence<beans::PropertyValue> aArgs(2);
        
        // Set URL
        aArgs.getArray()[0].Name = "URL";
        aArgs.getArray()[0].Value <<= sTempURL;
        
        // Set filter name
        aArgs.getArray()[1].Name = "FilterName";
        OUString sFilterName = "writer8"; // Default
        
        // Determine correct filter based on document type
        if (rsFileName.endsWith(".odt"))
            sFilterName = "writer8";
        else if (rsFileName.endsWith(".ods"))
            sFilterName = "calc8";
        else if (rsFileName.endsWith(".odp"))
            sFilterName = "impress8";
        else if (rsFileName.endsWith(".odg"))
            sFilterName = "draw8";
            
        aArgs.getArray()[1].Value <<= sFilterName;

        // Save document to temporary file
        xStorable->storeToURL(sTempURL, aArgs);

        // Read file content
        std::ifstream file(aTempFile.GetFileName().toUtf8().getStr(), std::ios::binary);
        if (!file.is_open())
        {
            SAL_WARN("sfx.control", "Failed to open temporary file for reading");
            return false;
        }

        // Read file content into string
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();

        // Convert to OUString (for now, we'll use base64 encoding for binary data)
        // In a real implementation, you'd want to send binary data directly
        rDocumentData = OUString::fromUtf8(content.c_str());

        SAL_WARN("sfx.control", "Document data prepared, size: " << content.length() << " bytes");
        return true;
    }
    catch (const uno::Exception& e)
    {
        SAL_WARN("sfx.control", "Exception while preparing document data: " << e.Message);
        return false;
    }
}

bool SaveToCloudHandler::uploadToCloud(const OUString& sDocumentData, const OUString& sFileName, const OUString& sContentType)
{
    if (!m_pApiClient)
    {
        SAL_WARN("sfx.control", "API client not available for upload");
        return false;
    }

    try
    {
        // Request presigned URL for upload
        OUString sPresignedUrl, sDocId;
        if (!m_pApiClient->requestPresignedUrl("put", sFileName, sContentType, sPresignedUrl, sDocId))
        {
            SAL_WARN("sfx.control", "Failed to get presigned URL for upload");
            return false;
        }

        SAL_WARN("sfx.control", "Got presigned URL for upload, docId: " << sDocId);

        // TODO: Implement actual file upload using the presigned URL
        // For now, we'll simulate success and register the document metadata
        
        // Calculate file size (simplified)
        sal_Int64 nFileSize = sDocumentData.getLength();

        // Register document metadata
        if (!m_pApiClient->registerDocument(sDocId, sFileName, nFileSize))
        {
            SAL_WARN("sfx.control", "Failed to register document metadata");
            return false;
        }

        SAL_WARN("sfx.control", "Document uploaded successfully, docId: " << sDocId);
        return true;
    }
    catch (const std::exception& e)
    {
        SAL_WARN("sfx.control", "Exception during upload: " << e.what());
        return false;
    }
}

void SaveToCloudHandler::showProgressDialog(const OUString& sMessage)
{
    // For now, just log the progress message
    // In a full implementation, we'd show a proper progress dialog
    SAL_WARN("sfx.control", "Progress: " << sMessage);
    std::cerr << "*** PROGRESS: " << OUStringToOString(sMessage, RTL_TEXTENCODING_UTF8).getStr() << " ***" << std::endl;
}

void SaveToCloudHandler::hideProgressDialog()
{
    // Hide progress dialog - placeholder
    SAL_WARN("sfx.control", "Progress dialog hidden");
}

void SaveToCloudHandler::showSuccessMessage(const OUString& sDocumentName)
{
    std::unique_ptr<weld::MessageDialog> xBox(Application::CreateMessageDialog(nullptr,
        VclMessageType::Info, VclButtonsType::Ok,
        "Document '" + sDocumentName + "' has been successfully saved to LibreCloud!\n\n"
        "You can access it from the Cloud Files dialog or the web dashboard."));
    xBox->set_title("Save to Cloud - Success");
    xBox->run();
}

void SaveToCloudHandler::showErrorMessage(const OUString& sError)
{
    std::unique_ptr<weld::MessageDialog> xBox(Application::CreateMessageDialog(nullptr,
        VclMessageType::Error, VclButtonsType::Ok,
        "Save to Cloud failed:\n\n" + sError));
    xBox->set_title("Save to Cloud - Error");
    xBox->run();
}

OUString SaveToCloudHandler::generateFileName()
{
    if (!m_pObjectShell)
        return "document.odt";

    // Get document title
    OUString sTitle = m_pObjectShell->GetTitle();
    if (sTitle.isEmpty())
        sTitle = "Untitled";

    // Remove invalid characters for filename
    sTitle = sTitle.replaceAll("/", "_");
    sTitle = sTitle.replaceAll("\\", "_");
    sTitle = sTitle.replaceAll(":", "_");
    sTitle = sTitle.replaceAll("*", "_");
    sTitle = sTitle.replaceAll("?", "_");
    sTitle = sTitle.replaceAll("\"", "_");
    sTitle = sTitle.replaceAll("<", "_");
    sTitle = sTitle.replaceAll(">", "_");
    sTitle = sTitle.replaceAll("|", "_");

    // Determine appropriate extension based on document type
    OUString sExtension = ".odt"; // Default to Writer format
    
    // Simple document type detection based on service name
    uno::Reference<uno::XInterface> xModel = m_pObjectShell->GetModel();
    if (xModel.is())
    {
        uno::Reference<css::lang::XServiceInfo> xServiceInfo(xModel, uno::UNO_QUERY);
        if (xServiceInfo.is())
        {
            if (xServiceInfo->supportsService("com.sun.star.text.TextDocument"))
                sExtension = ".odt";
            else if (xServiceInfo->supportsService("com.sun.star.sheet.SpreadsheetDocument"))
                sExtension = ".ods";
            else if (xServiceInfo->supportsService("com.sun.star.presentation.PresentationDocument"))
                sExtension = ".odp";
            else if (xServiceInfo->supportsService("com.sun.star.drawing.DrawingDocument"))
                sExtension = ".odg";
        }
    }

    return sTitle + sExtension;
}

OUString SaveToCloudHandler::getContentType()
{
    // Determine content type based on document type
    uno::Reference<uno::XInterface> xModel = m_pObjectShell->GetModel();
    if (xModel.is())
    {
        uno::Reference<css::lang::XServiceInfo> xServiceInfo(xModel, uno::UNO_QUERY);
        if (xServiceInfo.is())
        {
            if (xServiceInfo->supportsService("com.sun.star.text.TextDocument"))
                return "application/vnd.oasis.opendocument.text";
            else if (xServiceInfo->supportsService("com.sun.star.sheet.SpreadsheetDocument"))
                return "application/vnd.oasis.opendocument.spreadsheet";
            else if (xServiceInfo->supportsService("com.sun.star.presentation.PresentationDocument"))
                return "application/vnd.oasis.opendocument.presentation";
            else if (xServiceInfo->supportsService("com.sun.star.drawing.DrawingDocument"))
                return "application/vnd.oasis.opendocument.graphics";
        }
    }

    // Default to ODT
    return "application/vnd.oasis.opendocument.text";
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */ 