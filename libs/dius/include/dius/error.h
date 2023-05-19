#pragma once

#include <di/container/algorithm/max.h>
#include <di/vocab/error/status_code.h>
#include <dius/config.h>

#include DIUS_PLATFORM_PATH(error.h)

namespace di::platform {
using BasicError = dius::PosixError;

class GenericDomain;

using GenericCode = vocab::StatusCode<GenericDomain>;

class GenericDomain final : public vocab::StatusCodeDomain {
private:
    using Base = vocab::StatusCodeDomain;

public:
    using Value = BasicError;
    using UniqueId = Base::UniqueId;

    constexpr explicit GenericDomain(UniqueId id = 0xff261d32b71e0a8a) : Base(id) {}

    GenericDomain(GenericDomain const&) = default;
    GenericDomain(GenericDomain&&) = default;

    GenericDomain& operator=(GenericDomain const&) = default;
    GenericDomain& operator=(GenericDomain&&) = default;

    constexpr static inline GenericDomain const& get();

    virtual di::container::ErasedString name() const override { return container::ErasedString(u8"Posix) Domain"); }

    virtual PayloadInfo payload_info() const override {
        return { sizeof(Value), sizeof(Value) + sizeof(StatusCodeDomain const*),
                 di::container::max(alignof(Value), alignof(StatusCodeDomain const*)) };
    }

protected:
    constexpr virtual bool do_failure(vocab::StatusCode<void> const& code) const override {
        return down_cast(code).value() != BasicError::Success;
    }

    constexpr virtual bool do_equivalent(vocab::StatusCode<void> const& a,
                                         vocab::StatusCode<void> const& b) const override {
        DI_ASSERT(a.domain() == *this);
        return b.domain() == *this && down_cast(a).value() == down_cast(b).value();
    }

    constexpr virtual vocab::GenericCode do_convert_to_generic(vocab::StatusCode<void> const& a) const override {
        DI_ASSERT(a.domain() == *this);
        return vocab::GenericCode(di::in_place, down_cast(a).value());
    }

    constexpr virtual di::container::ErasedString do_message(vocab::StatusCode<void> const& code) const override {
        auto value = down_cast(code).value();
        if (value == BasicError::Success) {
            return container::ErasedString(u8"Success");
        }
        if (value == BasicError::AddressFamilyNotSupported) {
            return container::ErasedString(u8"Address familty not supported");
        }
        if (value == BasicError::AddressInUse) {
            return container::ErasedString(u8"Address in use");
        }
        if (value == BasicError::AddressNotAvailable) {
            return container::ErasedString(u8"Address not available");
        }
        if (value == BasicError::AlreadyConnected) {
            return container::ErasedString(u8"Already connected");
        }
        if (value == BasicError::ArgumentListTooLong) {
            return container::ErasedString(u8"Argument list too long");
        }
        if (value == BasicError::ArgumentOutOfDomain) {
            return container::ErasedString(u8"Argument out of domain");
        }
        if (value == BasicError::BadAddress) {
            return container::ErasedString(u8"Bad address");
        }
        if (value == BasicError::BadFileDescriptor) {
            return container::ErasedString(u8"Bad file descriptor");
        }
        if (value == BasicError::BadMessage) {
            return container::ErasedString(u8"Bad message");
        }
        if (value == BasicError::BrokenPipe) {
            return container::ErasedString(u8"Broken pipe");
        }
        if (value == BasicError::ConnectionAborted) {
            return container::ErasedString(u8"Connect aborted");
        }
        if (value == BasicError::ConnectionAlreadyInProgress) {
            return container::ErasedString(u8"Connection already in progress");
        }
        if (value == BasicError::ConnectionRefused) {
            return container::ErasedString(u8"Connection refused");
        }
        if (value == BasicError::ConnectionReset) {
            return container::ErasedString(u8"Connection reset");
        }
        if (value == BasicError::CrossDeviceLink) {
            return container::ErasedString(u8"Cross device link");
        }
        if (value == BasicError::DestinationAddressRequired) {
            return container::ErasedString(u8"Destination address required");
        }
        if (value == BasicError::DeviceOrResourceBusy) {
            return container::ErasedString(u8"Device or resource busy");
        }
        if (value == BasicError::DirectoryNotEmpty) {
            return container::ErasedString(u8"Directory not empty");
        }
        if (value == BasicError::ExecutableFormatError) {
            return container::ErasedString(u8"Executable format error");
        }
        if (value == BasicError::FileExists) {
            return container::ErasedString(u8"File exists");
        }
        if (value == BasicError::FileTooLarge) {
            return container::ErasedString(u8"File too large");
        }
        if (value == BasicError::FilenameTooLong) {
            return container::ErasedString(u8"Filename too long");
        }
        if (value == BasicError::FunctionNotSupported) {
            return container::ErasedString(u8"Function not supported");
        }
        if (value == BasicError::HostUnreachable) {
            return container::ErasedString(u8"Host unreachable");
        }
        if (value == BasicError::IdentifierRemoved) {
            return container::ErasedString(u8"Identifier removed");
        }
        if (value == BasicError::IllegalByteSequence) {
            return container::ErasedString(u8"Illegal byte sequence");
        }
        if (value == BasicError::InappropriateIoControlOperation) {
            return container::ErasedString(u8"Inappropriate io control operation");
        }
        if (value == BasicError::Interrupted) {
            return container::ErasedString(u8"Interrupted");
        }
        if (value == BasicError::InvalidArgument) {
            return container::ErasedString(u8"Invalid argument");
        }
        if (value == BasicError::InvalidSeek) {
            return container::ErasedString(u8"Invalid seek");
        }
        if (value == BasicError::IoError) {
            return container::ErasedString(u8"IO error");
        }
        if (value == BasicError::IsADirectory) {
            return container::ErasedString(u8"Is a directory");
        }
        if (value == BasicError::MessageSize) {
            return container::ErasedString(u8"Message size");
        }
        if (value == BasicError::NetworkDown) {
            return container::ErasedString(u8"Network down");
        }
        if (value == BasicError::NetworkReset) {
            return container::ErasedString(u8"Network reset");
        }
        if (value == BasicError::NetworkUnreachable) {
            return container::ErasedString(u8"Network unreachable");
        }
        if (value == BasicError::NoBufferSpace) {
            return container::ErasedString(u8"No buffer space");
        }
        if (value == BasicError::NoChildProcess) {
            return container::ErasedString(u8"No child process");
        }
        if (value == BasicError::NoLink) {
            return container::ErasedString(u8"No link");
        }
        if (value == BasicError::NoLockAvailable) {
            return container::ErasedString(u8"No lock available");
        }
        if (value == BasicError::NoMessageAvailable) {
            return container::ErasedString(u8"No message available");
        }
        if (value == BasicError::NoMessage) {
            return container::ErasedString(u8"No message");
        }
        if (value == BasicError::NoProtocolOption) {
            return container::ErasedString(u8"No protocol option");
        }
        if (value == BasicError::NoSpaceOnDevice) {
            return container::ErasedString(u8"No space on device");
        }
        if (value == BasicError::NoStreamResources) {
            return container::ErasedString(u8"No stream resources");
        }
        if (value == BasicError::NoSuchDeviceOrAddress) {
            return container::ErasedString(u8"No such device or address");
        }
        if (value == BasicError::NoSuchDevice) {
            return container::ErasedString(u8"No such device");
        }
        if (value == BasicError::NoSuchFileOrDirectory) {
            return container::ErasedString(u8"No such file or directory");
        }
        if (value == BasicError::NoSuchProcess) {
            return container::ErasedString(u8"No such process");
        }
        if (value == BasicError::NotADirectory) {
            return container::ErasedString(u8"Not a directory");
        }
        if (value == BasicError::NotASocket) {
            return container::ErasedString(u8"Not a socket");
        }
        if (value == BasicError::NotAStream) {
            return container::ErasedString(u8"Not a stream");
        }
        if (value == BasicError::NotConnected) {
            return container::ErasedString(u8"Not connected");
        }
        if (value == BasicError::NotEnoughMemory) {
            return container::ErasedString(u8"Not enough memory");
        }
        if (value == BasicError::OperationCanceled) {
            return container::ErasedString(u8"Operation cancelled");
        }
        if (value == BasicError::OperationInProgress) {
            return container::ErasedString(u8"Operation in progress");
        }
        if (value == BasicError::OperationNotPermitted) {
            return container::ErasedString(u8"Operation not permitted");
        }
        if (value == BasicError::NotSupported) {
            return container::ErasedString(u8"Not supported");
        }
        if (value == BasicError::OperationNotSupported) {
            return container::ErasedString(u8"Operation not supported");
        }
        if (value == BasicError::OperationWouldBlock) {
            return container::ErasedString(u8"Operation would block");
        }
        if (value == BasicError::OwnerDead) {
            return container::ErasedString(u8"Owner dead");
        }
        if (value == BasicError::PermissionDenied) {
            return container::ErasedString(u8"Permission denied");
        }
        if (value == BasicError::ProtocolError) {
            return container::ErasedString(u8"Protocol error");
        }
        if (value == BasicError::ProtocolNotSupported) {
            return container::ErasedString(u8"Protocol not supported");
        }
        if (value == BasicError::ReadOnlyFileSystem) {
            return container::ErasedString(u8"Read only file system");
        }
        if (value == BasicError::ResourceDeadlockWouldOccur) {
            return container::ErasedString(u8"Resource deadlock would occur");
        }
        if (value == BasicError::ResourceUnavailableTryAgain) {
            return container::ErasedString(u8"Resource unavailable try again");
        }
        if (value == BasicError::ResultOutOfRange) {
            return container::ErasedString(u8"Result out of range");
        }
        if (value == BasicError::StateNotRecoverable) {
            return container::ErasedString(u8"State not recoverable");
        }
        if (value == BasicError::StreamTimeout) {
            return container::ErasedString(u8"Stream timeout");
        }
        if (value == BasicError::TextFileBusy) {
            return container::ErasedString(u8"Text file busy");
        }
        if (value == BasicError::TimedOut) {
            return container::ErasedString(u8"Timed out");
        }
        if (value == BasicError::TooManyFilesOpenInSystem) {
            return container::ErasedString(u8"Too many files open in system");
        }
        if (value == BasicError::TooManyFilesOpen) {
            return container::ErasedString(u8"Too many files open");
        }
        if (value == BasicError::TooManyLinks) {
            return container::ErasedString(u8"Too many links");
        }
        if (value == BasicError::TooManySymbolicLinkLevels) {
            return container::ErasedString(u8"Too many symbolic link levels");
        }
        if (value == BasicError::ValueTooLarge) {
            return container::ErasedString(u8"Value too large");
        }
        if (value == BasicError::WrongProtocolType) {
            return container::ErasedString(u8"Wrong protocol type");
        }
        return container::ErasedString(u8"(Unknown)");
    }

private:
    template<typename Domain>
    friend class di::vocab::StatusCode;

    constexpr GenericCode const& down_cast(vocab::StatusCode<void> const& code) const {
        DI_ASSERT(code.domain() == *this);
        return static_cast<GenericCode const&>(code);
    }
};

#ifdef DI_SANITIZER
// When compiling with UBSAN, using the address of a constexpr inline variable fails.
// This includes checking for nullptr. To work around this, do not declare the variable
// as inline when compiling with a sanitizer.
// See: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=71962.
// As a side note, this means there will be multiple copies of the generic_domain object
// in a user's program. This is perfectly fine, since we make sure to compare domains by
// their unique id and not their address, which is necessary even for inline variables when
// in the presence of dynamic linking.
constexpr auto generic_domain = GenericDomain {};
#else
constexpr inline auto generic_domain = GenericDomain {};
#endif

constexpr inline GenericDomain const& GenericDomain::get() {
    return generic_domain;
}
}

namespace di::vocab::detail {
constexpr auto tag_invoke(di::types::Tag<di::vocab::into_status_code>, di::platform::BasicError error) {
    return di::platform::GenericCode(di::in_place, error);
}
}

namespace di::vocab {
constexpr GenericCode StatusCode<void>::generic_code() const {
    if (!this->empty()) {
        return this->domain().do_convert_to_generic(*this);
    }
    return GenericCode(di::in_place, platform::BasicError::InvalidArgument);
}
}
