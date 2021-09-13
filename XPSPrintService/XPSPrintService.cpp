#include <Windows.h>
#include <stdio.h>
#include <xpsprint.h>



int main(int argc, char* argv[]) {
    // Initialize the COM interface, if the application has not 
    //  already done so.
    HRESULT hr;


    if (FAILED(hr = CoInitializeEx(0, COINIT_MULTITHREADED)))
    {
        fwprintf(stderr,
            L"ERROR: CoInitializeEx failed with HRESULT 0x%X\n", hr);
        return 1;
    }

    // Create the completion event
    HANDLE completionEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!completionEvent)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        fwprintf(stderr,
            L"ERROR: Could not create completion event: %08X\n", hr);
    }

    LPCSTR printerName = "SEC84251901F45D";
    IXpsPrintJob *job = NULL;
    IXpsPrintJobStream *jobStream = NULL;
    IXpsOMPackageTarget *printContentReceiver = NULL;

     //Start an XPS Print Job
    if (FAILED(hr = StartXpsPrintJob1(
        (LPCWSTR)printerName,
        NULL,
        L"192.168.31.22",
        NULL,
        completionEvent,
        &job,
        &printContentReceiver
    )))
    {
        fwprintf(stderr,
            L"ERROR: Could not start XPS print job: %08X\n", hr);
    }

    // Create an XPS OM Object Factory. If one has already been 
    //  created by the application, a new one is not necessary.

    IXpsOMObjectFactory* xpsFactory = NULL;
    if (SUCCEEDED(hr))
    {
        if (FAILED(hr = CoCreateInstance(
            __uuidof(XpsOMObjectFactory),
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&xpsFactory))))
        {
            fwprintf(
                stderr,
                L"ERROR: Could not create XPS OM Object Factory: %08X\n",
                hr);
        }
    }
    // Create the Part URI for the Fixed Document Sequence. The
    //  Fixed Document Sequence is the top-level element in the
    //  package hierarchy of objects. There is one Fixed Document
    //  Sequence in an XPS document.
    //
    // The part name is not specified by the XML Paper Specification,
    //  however, the name used in this example is the part name
    //  used by convention.
    //

    IOpcPartUri* partUri = NULL;
    if (SUCCEEDED(hr))
    {
        if (FAILED(hr = xpsFactory->CreatePartUri(
            L"/FixedDocumentSequence.fdseq",
            &partUri)))
        {
            fwprintf(stderr,
                L"ERROR: Could not create part URI: %08X\n", hr);
        }
    }

    IXpsOMPackageWriter* packageWriter = NULL;

    // Create the package writer on the print job stream.
    if (SUCCEEDED(hr))
    {
        if (FAILED(hr = xpsFactory->CreatePackageWriterOnStream(
            jobStream,
            TRUE,
            XPS_INTERLEAVING_ON,
            partUri,
            NULL,
            NULL,
            NULL,
            NULL,
            &packageWriter
        )
        )
            )
        {
            fwprintf(
                stderr,
                L"ERROR: Could not create package writer: 0x%X\n",
                hr);
        }
    }

    // Release the part URI interface.
    if (partUri)
    {
        partUri->Release();
        partUri = NULL;
    }

    // Create the Part URI for the Fixed Document. The
  //  Fixed Document part contains the pages of the document. 
  //  There can be one or more Fixed Documents in an XPS document.
  //
  // The part name is not specified by the XML Paper Specification,
  //  however, the name format used in this example is the format 
  //  used by convention. The number "1" in this example must be 
  //  changed for each document in the package. For example, 1 
  //  for the first document, 2 for the second, and so on.
  //

    if (SUCCEEDED(hr))
    {
        if (FAILED(hr = xpsFactory->CreatePartUri(
            L"/Documents/1/FixedDocument.fdoc",
            &partUri)))
        {
            fwprintf(
                stderr,
                L"ERROR: Could not create part URI: %08X\n",
                hr);
        }
    }

    // Start the new document.
    //
    //  If there was already a document started in this page,
    //  this call will close it and start a new one.
    if (SUCCEEDED(hr))
    {
        if (FAILED(hr = packageWriter->StartNewDocument(
            partUri,
            NULL,
            NULL,
            NULL,
            NULL)))
        {
            fwprintf(
                stderr,
                L"ERROR: Could not start new document: 0x%X\n",
                hr);
        }
    }

    // Release the part URI interface
    if (partUri)
    {
        partUri->Release();
        partUri = NULL;
    }

    IXpsOMPage* xpsPage = NULL;
    XPS_SIZE pageSize;
    pageSize.height = 1;    // 페이지 사이즈 설정
    pageSize.width = 1;
    if (SUCCEEDED(hr))
    {
        // Add the current page to the document.
        if (FAILED(hr = packageWriter->AddPage(
            xpsPage,
            &pageSize,
            NULL,
            NULL,
            NULL,
            NULL
        )))
        {
            fwprintf(
                stderr,
                L"ERROR: Could not add page to document: %08X\n",
                hr);
        }
    }

    if (SUCCEEDED(hr))
    {
        if (FAILED(hr = jobStream->Close()))
        {
            fwprintf(
                stderr,
                L"ERROR: Could not close job stream: %08X\n",
                hr);
        }
    }
    else
    {
        // Only cancel the job if we succeeded in creating a job.
        if (job)
        {
            // Tell the XPS Print API that we're giving up.  
            //  Don't overwrite hr with the return from this function.
            job->Cancel();
        }
    }

    if (SUCCEEDED(hr))
    {
        wprintf(L"Waiting for job completion...\n");

        if (WaitForSingleObject(completionEvent, INFINITE) !=
            WAIT_OBJECT_0)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            fwprintf(
                stderr,
                L"ERROR: Wait for completion event failed: %08X\n",
                hr);
        }
    }

    XPS_JOB_STATUS jobStatus;

    if (SUCCEEDED(hr))
    {
        if (FAILED(hr = job->GetJobStatus(&jobStatus)))
        {
            fwprintf(
                stderr,
                L"ERROR: Could not get job status: %08X\n",
                hr);
        }
    }

    if (SUCCEEDED(hr))
    {
        switch (jobStatus.completion)
        {
        case XPS_JOB_COMPLETED:
            break;
        case XPS_JOB_CANCELLED:
            fwprintf(stderr, L"ERROR: job was cancelled\n");
            hr = E_FAIL;
            break;
        case XPS_JOB_FAILED:
            fwprintf(
                stderr,
                L"ERROR: Print job failed: %08X\n",
                jobStatus.jobStatus);
            hr = E_FAIL;
            break;
        default:
            fwprintf(stderr, L"ERROR: unexpected failure\n");
            hr = E_UNEXPECTED;
            break;
        }
    }

    if (packageWriter)
    {
        packageWriter->Release();
        packageWriter = NULL;
    }

    if (partUri)
    {
        partUri->Release();
        partUri = NULL;
    }

    if (xpsFactory)
    {
        xpsFactory->Release();
        xpsFactory = NULL;
    }

    if (jobStream)
    {
        jobStream->Release();
        jobStream = NULL;
    }

    if (job)
    {
        job->Release();
        job = NULL;
    }

    if (completionEvent)
    {
        CloseHandle(completionEvent);
        completionEvent = NULL;
    }

    return 0;
}