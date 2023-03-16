#pragma once
#ifndef PM_CAMERA_H
#define PM_CAMERA_H

/* System */
#include <string>

/* PVCAM */
#include <master.h>
#include <pvcam.h>

/* Local */
#include "Settings.h"

// Function used as an interface between the queue and the callback
using CallbackEx3Fn = void (*)(FRAME_INFO* frameInfo, void* context);

namespace pm {

class Frame;

typedef struct
{
    int32 value;
    std::string desc;
}
EnumItem;

// The base class for all kind of cameras
class Camera
{
public:
    Camera();
    virtual ~Camera();

    // Library related methods

    // Initialize camera/library
    virtual bool Initialize() = 0;
    // Uninitialize camera/library
    virtual bool Uninitialize() = 0;
    // Current init state
    virtual bool IsInitialized() const = 0;

    // Get number of cameras detected
    virtual bool GetCameraCount(int16& count) const = 0;
    // Get name of the camera on given index
    virtual bool GetName(int16 index, std::string& name) const = 0;

    // Camera related methods

    // Get error message
    virtual std::string GetErrorMessage() const = 0;

    // Open camera
    virtual bool Open(const std::string& name) = 0;
    // Close camera
    virtual bool Close() = 0;
    // Current open state
    virtual bool IsOpen() const
    { return m_isOpen; }

    // Update all read-only settings parameter
    virtual bool UpdateReadOnlySettings(Settings& settings);
    // Return settings set via SetupExp
    const SettingsReader& GetSettings() const
    { return m_settings; }
    // Setup acquisition
    virtual bool SetupExp(const SettingsReader& settings);
    // Start acquisition
    virtual bool StartExp(CallbackEx3Fn callbackHandler, void* callbackContext) = 0;
    // Stop acquisition
    virtual bool StopExp() = 0;

    // Used to generically access the "set_param" api of PVCAM
    virtual bool SetParam(uns32 id, void* param) = 0;
    // Used to generically access the "get_param" api of PVCAM
    virtual bool GetParam(uns32 id, int16 attr, void* param) const = 0;
    // Used to get all enum items via PVCAM api
    virtual bool GetEnumParam(uns32 id, std::vector<EnumItem>& items) const = 0;

    // Get the latest frame and deliver it to the frame being pushed into the queue
    virtual bool GetLatestFrame(Frame& frame) const = 0;
    // Get the frame at index (should be used for displaying only)
    virtual bool GetFrameAt(size_t index, Frame& frame) const;
    // Get index of the frame from circular buffer
    virtual bool GetFrameIndex(const Frame& frame, size_t& index) const;

    // Get size of one frame in bytes
    uns32 GetFrameBytes() const
    { return m_frameBytes; }
    // Get size of whole buffer in bytes (frame_count = buffer_bytes / frame_bytes)
    uns32 GetBufferBytes() const
    { return m_bufferBytes; }

protected:
    // Allocate buffer for m_buffer
    virtual bool AllocateBuffer(uns32 bufferBytes);
    // Make sure the buffer is freed and the head pointer is chained at NULL
    virtual void DeleteBuffer();

protected:
    bool m_isOpen;
    SettingsReader m_settings;

    uns32 m_frameBytes; // Number of bytes in one frame in buffer
    void* m_buffer; // PVCAM buffer
    uns32 m_bufferBytes; // Number of bytes in buffer (circ/sequence)
    std::vector<FRAME_INFO*> m_fiBuffer; // FRAME_INFO buffer
};

} // namespace pm

#endif /* PM_CAMERA_H */
