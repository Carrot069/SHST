/*
 * Copyright by Photometrics
 */

// System

// Local
#include "Common.h"

// This is the function registered as callback function and PVCAM will call it
// when new frame arrives
void NewFrameHandler(void)
{
    // Unblock main thread
    {
        std::lock_guard<std::mutex> lock(g_EofMutex);
        g_EofFlag = true; // Set flag
    }
    g_EofCond.notify_one();
}

// This sample application demonstrates acquisition of single frames with
// callback notification in a loop which collects 5 frames.
// Each acquisition is started by the host with a software trigger
// (pl_exp_start_seq()).
// Please note pl_exp_finish_seq() needs to be called after each frame is
// acquired before new software trigger pl_exp_start_seq() is sent.
int main(int argc, char* argv[])
{
    if (!ShowAppInfo(argc, argv))
        return APP_EXIT_ERROR;

    if (!InitAndOpenFirstCamera())
        return APP_EXIT_ERROR;

    // Set the CCD g_Region fo Full Frame, serial and parallel binning is set to 1
    g_Region.s1 = 0;
    g_Region.s2 = g_SensorResX - 1;
    g_Region.sbin = 1;
    g_Region.p1 = 0;
    g_Region.p2 = g_SensorResY - 1;
    g_Region.pbin = 1;

    // Register the callback function that will be called by PVCAM when the
    // specified event - in this case EOF (end of frame/end of readout) arrives
    // Some interfaces, such as legacy PCI LVDS interface, do not support
    // callbacks and an error will be returned here for those interfaces.
    if (PV_OK != pl_cam_register_callback(g_hCam, PL_CALLBACK_EOF,
            (void *)NewFrameHandler))
    {
        PrintErrorMessage(pl_error_code(), "pl_cam_register_callback() error");
        CloseCameraAndUninit();
        return APP_EXIT_ERROR;
    }
    printf("Callback registered successfully\n");

    uns32 exposureBytes;
    const uns32 exposureTime = 40; // milliseconds

    // Setup the acquisition
    // TIMED_MODE flag means acquisition will start with software trigger,
    // other flags such as STROBED_MODE can be used for hardware trigger.
    if (PV_OK != pl_exp_setup_seq(g_hCam, 1, 1, &g_Region, TIMED_MODE,
            exposureTime, &exposureBytes))
    {
        PrintErrorMessage(pl_error_code(), "pl_exp_setup_seq() error");
        CloseCameraAndUninit();
        return APP_EXIT_ERROR;
    }
    printf("Acquisition setup successful\n");

    // Allocate frame memory assuming this is a 16-bit camera (or more than 8 bit)
    // Since each pixel is 2 bytes allocate uns16 array, therefore divide the size
    // in bytes returned by pl_exp_setup_seq() by 2.
    uns16 *frameInMemory =
        new (std::nothrow) uns16[exposureBytes / sizeof(uns16)];
    if (frameInMemory == NULL)
    {
        printf("Unable to allocate memory\n");
        CloseCameraAndUninit();
        return APP_EXIT_ERROR;
    }

    // Loop acquiring 5 images
    rs_bool errorOccured = FALSE;
    uns32 imageCounter = 0;
    while (imageCounter < 5)
    {
        // Start the acqusition - it is used as software trigger in TIMED
        // trigger mode. In hardware trigger mode (Strobe or Bulb) after this
        // call camera waits for external trigger signal.
        if (PV_OK != pl_exp_start_seq(g_hCam, frameInMemory))
        {
            PrintErrorMessage(pl_error_code(), "pl_exp_start_seq() error");
            errorOccured = TRUE;
            break;
        }
        printf("Acquisition start successful\n");

        // Here we need to wait for a frame readout notification signaled by
        // g_EofCond which is raised in the NewFrameHandler() callback.
        printf("Waiting for EOF event to occur\n");
        {
            std::unique_lock<std::mutex> lock(g_EofMutex);
            if (!g_EofFlag)
            {
                g_EofCond.wait_for(lock, std::chrono::seconds(5), []() {
                    return (g_EofFlag);
                });
            }
            if (!g_EofFlag)
            {
                printf("Camera timed out waiting for a frame\n");
                errorOccured = TRUE;
                break;
            }
            g_EofFlag = false; // Reset flag
        }

        printf("Frame #%d has been delivered\n", imageCounter + 1);
        ShowImage(frameInMemory, exposureBytes / sizeof(uns16), NULL);

        // When acquiring single frames with callback notifications call this
        // after each frame before new acquisition is started with pl_exp_start_seq()
        if (PV_OK != pl_exp_finish_seq(g_hCam, frameInMemory, 0))
            PrintErrorMessage(pl_error_code(), "pl_exp_finish_seq() error");

        imageCounter++;
    }

    if (PV_OK != pl_exp_abort(g_hCam, CCS_NO_CHANGE))
        PrintErrorMessage(pl_error_code(), "pl_exp_abort(CCS_NO_CHANGE) error");

    CloseCameraAndUninit();

    delete [] frameInMemory;

    if (errorOccured)
        return APP_EXIT_ERROR;
    return 0;
}
