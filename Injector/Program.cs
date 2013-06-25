using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Syringe;
using System.IO.Pipes;
using System.IO;
using System.Threading.Tasks;
using System.Threading;
using System.Runtime.InteropServices;

using D3DHookingLibrary;

namespace DLLTestUtil
{
    class Program
    {
        [StructLayout(LayoutKind.Sequential)]
        public struct MessageStruct
        {
            public IntPtr dip_address;
            public IntPtr setIndices_address;
            public IntPtr setStreamSource_address;
            public IntPtr setVertexShader_address;
        }

       
        static void Main(string[] args)
        {
            Executor injector = new Executor();
            string dllName = "D3DModelRipper.dll";
            injector.Inject(dllName, args[0].Substring(0, args[0].Length-4));

            D3DFuncLookup d3d9Util = new D3DHookingLibrary.D3DFuncLookup(args[0], args[1]);
            
            IntPtr dipAddress = (IntPtr)d3d9Util.GetD3DFunction(D3DFuncLookup.D3D9Functions.DrawIndexedPrimitive);
            //IntPtr setIndicesAddress = (IntPtr)d3d9Util.GetD3DFunction(D3DFuncLookup.D3D9Functions.SetIndices);
            IntPtr setStreamSourceAddress = (IntPtr)d3d9Util.GetD3DFunction(D3DFuncLookup.D3D9Functions.SetStreamSource);
            IntPtr setVertexShaderAddress = (IntPtr)d3d9Util.GetD3DFunction(D3DFuncLookup.D3D9Functions.SetVertexShader);
            MessageStruct mes = new MessageStruct() { dip_address = dipAddress, setIndices_address = setIndicesAddress, 
                                                      setStreamSource_address = setStreamSourceAddress, 
                                                      setVertexShader_address = setVertexShaderAddress};
            injector.getSyringe().CallExport(dllName, "InstallHook", mes);
            while (true) Thread.Sleep(300);
        }

    }
}
