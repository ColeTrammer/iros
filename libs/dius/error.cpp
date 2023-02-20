#include <dius/error.h>

namespace dius {
di::ErasedString PosixDomain::do_message(di::StatusCode<void> const& code) const {
    auto value = down_cast(code).value();
    if (value == PosixError::Success) {
        return "Success"_sv;
    }
    if (value == PosixError::AddressFamilyNotSupported) {
        return "Address familty not supported"_sv;
    }
    if (value == PosixError::AddressInUse) {
        return "Address in use"_sv;
    }
    if (value == PosixError::AddressNotAvailable) {
        return "Address not available"_sv;
    }
    if (value == PosixError::AlreadyConnected) {
        return "Already connected"_sv;
    }
    if (value == PosixError::ArgumentListTooLong) {
        return "Argument list too long"_sv;
    }
    if (value == PosixError::ArgumentOutOfDomain) {
        return "Argument out of domain"_sv;
    }
    if (value == PosixError::BadAddress) {
        return "Bad address"_sv;
    }
    if (value == PosixError::BadFileDescriptor) {
        return "Bad file descriptor"_sv;
    }
    if (value == PosixError::BadMessage) {
        return "Bad message"_sv;
    }
    if (value == PosixError::BrokenPipe) {
        return "Broken pipe"_sv;
    }
    if (value == PosixError::ConnectionAborted) {
        return "Connect aborted"_sv;
    }
    if (value == PosixError::ConnectionAlreadyInProgress) {
        return "Connection already in progress"_sv;
    }
    if (value == PosixError::ConnectionRefused) {
        return "Connection refused"_sv;
    }
    if (value == PosixError::ConnectionReset) {
        return "Connection reset"_sv;
    }
    if (value == PosixError::CrossDeviceLink) {
        return "Cross device link"_sv;
    }
    if (value == PosixError::DestinationAddressRequired) {
        return "Destination address required"_sv;
    }
    if (value == PosixError::DeviceOrResourceBusy) {
        return "Device or resource busy"_sv;
    }
    if (value == PosixError::DirectoryNotEmpty) {
        return "Directory not empty"_sv;
    }
    if (value == PosixError::ExecutableFormatError) {
        return "Executable format error"_sv;
    }
    if (value == PosixError::FileExists) {
        return "File exists"_sv;
    }
    if (value == PosixError::FileTooLarge) {
        return "File too large"_sv;
    }
    if (value == PosixError::FilenameTooLong) {
        return "Filename too long"_sv;
    }
    if (value == PosixError::FunctionNotSupported) {
        return "Function not supported"_sv;
    }
    if (value == PosixError::HostUnreachable) {
        return "Host unreachable"_sv;
    }
    if (value == PosixError::IdentifierRemoved) {
        return "Identifier removed"_sv;
    }
    if (value == PosixError::IllegalByteSequence) {
        return "Illegal byte sequence"_sv;
    }
    if (value == PosixError::InappropriateIoControlOperation) {
        return "Inappropriate io control operation"_sv;
    }
    if (value == PosixError::Interrupted) {
        return "Interrupted"_sv;
    }
    if (value == PosixError::InvalidArgument) {
        return "Invalid argument"_sv;
    }
    if (value == PosixError::InvalidSeek) {
        return "Invalid seek"_sv;
    }
    if (value == PosixError::IoError) {
        return "IO error"_sv;
    }
    if (value == PosixError::IsADirectory) {
        return "Is a directory"_sv;
    }
    if (value == PosixError::MessageSize) {
        return "Message size"_sv;
    }
    if (value == PosixError::NetworkDown) {
        return "Network down"_sv;
    }
    if (value == PosixError::NetworkReset) {
        return "Network reset"_sv;
    }
    if (value == PosixError::NetworkUnreachable) {
        return "Network unreachable"_sv;
    }
    if (value == PosixError::NoBufferSpace) {
        return "No buffer space"_sv;
    }
    if (value == PosixError::NoChildProcess) {
        return "No child process"_sv;
    }
    if (value == PosixError::NoLink) {
        return "No link"_sv;
    }
    if (value == PosixError::NoLockAvailable) {
        return "No lock available"_sv;
    }
    if (value == PosixError::NoMessageAvailable) {
        return "No message available"_sv;
    }
    if (value == PosixError::NoMessage) {
        return "No message"_sv;
    }
    if (value == PosixError::NoProtocolOption) {
        return "No protocol option"_sv;
    }
    if (value == PosixError::NoSpaceOnDevice) {
        return "No space on device"_sv;
    }
    if (value == PosixError::NoStreamResources) {
        return "No stream resources"_sv;
    }
    if (value == PosixError::NoSuchDeviceOrAddress) {
        return "No such device or address"_sv;
    }
    if (value == PosixError::NoSuchDevice) {
        return "No such device"_sv;
    }
    if (value == PosixError::NoSuchFileOrDirectory) {
        return "No such file or directory"_sv;
    }
    if (value == PosixError::NoSuchProcess) {
        return "No such process"_sv;
    }
    if (value == PosixError::NotADirectory) {
        return "Not a directory"_sv;
    }
    if (value == PosixError::NotASocket) {
        return "Not a socket"_sv;
    }
    if (value == PosixError::NotAStream) {
        return "Not a stream"_sv;
    }
    if (value == PosixError::NotConnected) {
        return "Not connected"_sv;
    }
    if (value == PosixError::NotEnoughMemory) {
        return "Not enough memory"_sv;
    }
    if (value == PosixError::OperationCanceled) {
        return "Operation cancelled"_sv;
    }
    if (value == PosixError::OperationInProgress) {
        return "Operation in progress"_sv;
    }
    if (value == PosixError::OperationNotPermitted) {
        return "Operation not permitted"_sv;
    }
    if (value == PosixError::NotSupported) {
        return "Not supported"_sv;
    }
    if (value == PosixError::OperationNotSupported) {
        return "Operation not supported"_sv;
    }
    if (value == PosixError::OperationWouldBlock) {
        return "Operation would block"_sv;
    }
    if (value == PosixError::OwnerDead) {
        return "Owner dead"_sv;
    }
    if (value == PosixError::PermissionDenied) {
        return "Permission denied"_sv;
    }
    if (value == PosixError::ProtocolError) {
        return "Protocol error"_sv;
    }
    if (value == PosixError::ProtocolNotSupported) {
        return "Protocol not supported"_sv;
    }
    if (value == PosixError::ReadOnlyFileSystem) {
        return "Read only file system"_sv;
    }
    if (value == PosixError::ResourceDeadlockWouldOccur) {
        return "Resource deadlock would occur"_sv;
    }
    if (value == PosixError::ResourceUnavailableTryAgain) {
        return "Resource unavailable try again"_sv;
    }
    if (value == PosixError::ResultOutOfRange) {
        return "Result out of range"_sv;
    }
    if (value == PosixError::StateNotRecoverable) {
        return "State not recoverable"_sv;
    }
    if (value == PosixError::StreamTimeout) {
        return "Stream timeout"_sv;
    }
    if (value == PosixError::TextFileBusy) {
        return "Text file busy"_sv;
    }
    if (value == PosixError::TimedOut) {
        return "Timed out"_sv;
    }
    if (value == PosixError::TooManyFilesOpenInSystem) {
        return "Too many files open in system"_sv;
    }
    if (value == PosixError::TooManyFilesOpen) {
        return "Too many files open"_sv;
    }
    if (value == PosixError::TooManyLinks) {
        return "Too many links"_sv;
    }
    if (value == PosixError::TooManySymbolicLinkLevels) {
        return "Too many symbolic link levels"_sv;
    }
    if (value == PosixError::ValueTooLarge) {
        return "Value too large"_sv;
    }
    if (value == PosixError::WrongProtocolType) {
        return "Wrong protocol type"_sv;
    }
    return "(Unknown)"_sv;
}
}