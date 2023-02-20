namespace iris {
enum class Error : long {
    Success = 0,
    AddressFamilyNotSupported = 1,
    AddressInUse = 2,
    AddressNotAvailable = 3,
    AlreadyConnected = 4,
    ArgumentListTooLong = 5,
    ArgumentOutOfDomain = 6,
    BadAddress = 7,
    BadFileDescriptor = 8,
    BadMessage = 9,
    BrokenPipe = 10,
    ConnectionAborted = 11,
    ConnectionAlreadyInProgress = 12,
    ConnectionRefused = 13,
    ConnectionReset = 14,
    CrossDeviceLink = 15,
    DestinationAddressRequired = 16,
    DeviceOrResourceBusy = 17,
    DirectoryNotEmpty = 18,
    ExecutableFormatError = 19,
    FileExists = 20,
    FileTooLarge = 21,
    FilenameTooLong = 22,
    FunctionNotSupported = 23,
    HostUnreachable = 24,
    IdentifierRemoved = 25,
    IllegalByteSequence = 26,
    InappropriateIoControlOperation = 27,
    Interrupted = 28,
    InvalidArgument = 29,
    InvalidSeek = 30,
    IoError = 31,
    IsADirectory = 32,
    MessageSize = 33,
    NetworkDown = 34,
    NetworkReset = 35,
    NetworkUnreachable = 36,
    NoBufferSpace = 37,
    NoChildProcess = 38,
    NoLink = 39,
    NoLockAvailable = 40,
    NoMessageAvailable = 41,
    NoMessage = 42,
    NoProtocolOption = 43,
    NoSpaceOnDevice = 44,
    NoStreamResources = 45,
    NoSuchDeviceOrAddress = 46,
    NoSuchDevice = 47,
    NoSuchFileOrDirectory = 48,
    NoSuchProcess = 49,
    NotADirectory = 50,
    NotASocket = 51,
    NotAStream = 52,
    NotConnected = 53,
    NotEnoughMemory = 54,
    NotSupported = 55,
    OperationCanceled = 56,
    OperationInProgress = 57,
    OperationNotPermitted = 58,
    OperationNotSupported = 59,
    OperationWouldBlock = 60,
    OwnerDead = 61,
    PermissionDenied = 62,
    ProtocolError = 63,
    ProtocolNotSupported = 64,
    ReadOnlyFileSystem = 65,
    ResourceDeadlockWouldOccur = 66,
    ResourceUnavailableTryAgain = 67,
    ResultOutOfRange = 68,
    StateNotRecoverable = 69,
    StreamTimeout = 70,
    TextFileBusy = 71,
    TimedOut = 72,
    TooManyFilesOpenInSystem = 73,
    TooManyFilesOpen = 74,
    TooManyLinks = 75,
    TooManySymbolicLinkLevels = 76,
    ValueTooLarge = 77,
    WrongProtocolType = 78,
};
}