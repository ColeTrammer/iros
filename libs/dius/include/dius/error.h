#pragma once

#include <di/prelude.h>
#include <errno.h>
#include <string.h>

namespace dius {
class PosixDomain;

using PosixCode = di::StatusCode<PosixDomain>;

inline namespace posix_error {
    enum class PosixError : long {
        Success = 0,
        AddressFamilyNotSupported = EAFNOSUPPORT,
        AddressInUse = EADDRINUSE,
        AddressNotAvailable = EADDRNOTAVAIL,
        AlreadyConnected = EISCONN,
        ArgumentListTooLong = E2BIG,
        ArgumentOutOfDomain = EDOM,
        BadAddress = EFAULT,
        BadFileDescriptor = EBADF,
        BadMessage = EBADMSG,
        BrokenPipe = EPIPE,
        ConnectionAborted = ECONNABORTED,
        ConnectionAlreadyInProgress = EALREADY,
        ConnectionRefused = ECONNREFUSED,
        ConnectionReset = ECONNRESET,
        CrossDeviceLink = EXDEV,
        DestinationAddressRequired = EDESTADDRREQ,
        DeviceOrResourceBusy = EBUSY,
        DirectoryNotEmpty = ENOTEMPTY,
        ExecutableFormatError = ENOEXEC,
        FileExists = EEXIST,
        FileTooLarge = EFBIG,
        FilenameTooLong = ENAMETOOLONG,
        FunctionNotSupported = ENOSYS,
        HostUnreachable = EHOSTUNREACH,
        IdentifierRemoved = EIDRM,
        IllegalByteSequence = EILSEQ,
        InappropriateIoControlOperation = ENOTTY,
        Interrupted = EINTR,
        InvalidArgument = EINVAL,
        InvalidSeek = ESPIPE,
        IoError = EIO,
        IsADirectory = EISDIR,
        MessageSize = EMSGSIZE,
        NetworkDown = ENETDOWN,
        NetworkReset = ENETRESET,
        NetworkUnreachable = ENETUNREACH,
        NoBufferSpace = ENOBUFS,
        NoChildProcess = ECHILD,
        NoLink = ENOLINK,
        NoLockAvailable = ENOLCK,
        NoMessageAvailable = ENODATA,
        NoMessage = ENOMSG,
        NoProtocolOption = ENOPROTOOPT,
        NoSpaceOnDevice = ENOSPC,
        NoStreamResources = ENOSR,
        NoSuchDeviceOrAddress = ENXIO,
        NoSuchDevice = ENODEV,
        NoSuchFileOrDirectory = ENOENT,
        NoSuchProcess = ESRCH,
        NotADirectory = ENOTDIR,
        NotASocket = ENOTSOCK,
        NotAStream = ENOSTR,
        NotConnected = ENOTCONN,
        NotEnoughMemory = ENOMEM,
        NotSupported = ENOTSUP,
        OperationCanceled = ECANCELED,
        OperationInProgress = EINPROGRESS,
        OperationNotPermitted = EPERM,
        OperationNotSupported = EOPNOTSUPP,
        OperationWouldBlock = EWOULDBLOCK,
        OwnerDead = EOWNERDEAD,
        PermissionDenied = EACCES,
        ProtocolError = EPROTO,
        ProtocolNotSupported = EPROTONOSUPPORT,
        ReadOnlyFileSystem = EROFS,
        ResourceDeadlockWouldOccur = EDEADLK,
        ResourceUnavailableTryAgain = EAGAIN,
        ResultOutOfRange = ERANGE,
        StateNotRecoverable = ENOTRECOVERABLE,
        StreamTimeout = ETIME,
        TextFileBusy = ETXTBSY,
        TimedOut = ETIMEDOUT,
        TooManyFilesOpenInSystem = ENFILE,
        TooManyFilesOpen = EMFILE,
        TooManyLinks = EMLINK,
        TooManySymbolicLinkLevels = ELOOP,
        ValueTooLarge = EOVERFLOW,
        WrongProtocolType = EPROTOTYPE,
    };

    template<typename = void>
    constexpr auto tag_invoke(di::Tag<di::into_status_code>, PosixError error) {
        return PosixCode(di::in_place, error);
    }
}

class PosixDomain final : public di::StatusCodeDomain {
private:
    using Base = StatusCodeDomain;

public:
    using Value = PosixError;
    using UniqueId = Base::UniqueId;

    constexpr explicit PosixDomain(UniqueId id = 0xff261d32b71e0a8a) : Base(id) {}

    PosixDomain(PosixDomain const&) = default;
    PosixDomain(PosixDomain&&) = default;

    PosixDomain& operator=(PosixDomain const&) = default;
    PosixDomain& operator=(PosixDomain&&) = default;

    constexpr static inline PosixDomain const& get();

    virtual di::ErasedString name() const override { return u8"Posix Domain"_sv; }

    virtual PayloadInfo payload_info() const override {
        return { sizeof(Value), sizeof(Value) + sizeof(StatusCodeDomain const*),
                 di::max(alignof(Value), alignof(StatusCodeDomain const*)) };
    }

protected:
    constexpr virtual bool do_failure(di::StatusCode<void> const& code) const override {
        return down_cast(code).value() != PosixError::Success;
    }

    constexpr virtual bool do_equivalent(di::StatusCode<void> const& a, di::StatusCode<void> const& b) const override {
        DI_ASSERT_EQ(a.domain(), *this);
        return b.domain() == *this && down_cast(a).value() == down_cast(b).value();
    }

    virtual di::ErasedString do_message(di::StatusCode<void> const& code) const override;

private:
    template<typename Domain>
    friend class StatusCode;

    constexpr PosixCode const& down_cast(di::StatusCode<void> const& code) const {
        DI_ASSERT_EQ(code.domain(), *this);
        return static_cast<PosixCode const&>(code);
    }
};

constexpr inline auto posix_domain = PosixDomain {};

constexpr inline PosixDomain const& PosixDomain::get() {
    return posix_domain;
}
}