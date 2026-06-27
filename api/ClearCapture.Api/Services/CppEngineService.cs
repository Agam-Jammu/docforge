using System.Runtime.InteropServices;
using System.Text.Json;

namespace ClearCapture.Api.Services;

/// <summary>
/// P/Invoke wrapper around the C++ ClearCapture engine shared library.
/// </summary>
public class CppEngineService : IDisposable
{
    private const string DllPath = "libclearcapture_engine";

    [DllImport(DllPath, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr CLEAR_Initialize(int numThreads);

    [DllImport(DllPath, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr CLEAR_ProcessDocument(string filePath, string documentType);

    [DllImport(DllPath, CallingConvention = CallingConvention.Cdecl)]
    private static extern void CLEAR_FreeString(IntPtr ptr);

    [DllImport(DllPath, CallingConvention = CallingConvention.Cdecl)]
    private static extern void CLEAR_Shutdown();

    public CppEngineService()
    {
        var result = CLEAR_Initialize(Environment.ProcessorCount);
        if (result != IntPtr.Zero)
        {
            var msg = Marshal.PtrToStringAnsi(result) ?? "Unknown error";
            CLEAR_FreeString(result);
            Console.WriteLine($"[CppEngine] Initialize: {msg}");
        }
    }

    public string? ProcessDocument(string filePath, string documentType)
    {
        var ptr = CLEAR_ProcessDocument(filePath, documentType);
        if (ptr == IntPtr.Zero) return null;
        var json = Marshal.PtrToStringAnsi(ptr);
        CLEAR_FreeString(ptr);
        return json;
    }

    public void Dispose()
    {
        CLEAR_Shutdown();
    }
}