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
            public IntPtr setStreamSource_address;
            public Int32 primcount;
            public Int32 vertnum;
        }

        static int ProcessParameters(string[] args, ref MessageStruct mes)
        {
            int retval = 0;
            if (args.Length < 4 || args.Length > 4)
            {
                Console.WriteLine("Usage: ExeName d3dX.dll PrimCount VertCount");
                retval = -1;
            }
            else
            {
                try
                {
                    mes.primcount = Convert.ToInt32(args[2]);
                    mes.vertnum = Convert.ToInt32(args[3]);
                }
                catch (Exception e)
                {
                    Console.WriteLine(e.Message);
                    retval = -2;
                }

            }
            return retval;
        }
       
        static void Main(string[] args)
        {
            Executor injector = new Executor();
            string dllName = "D3DModelRipper.dll";
            MessageStruct mes = new MessageStruct();
            if (ProcessParameters(args, ref mes) != 0)
            {
                Console.WriteLine("Error Processing Parameters");
                return;
            }

            try
            {
                injector.Inject(dllName, args[0].Substring(0, args[0].Length - 4));
            }
            catch (Exception e)
            {
                Console.WriteLine(e.Message);
                Console.WriteLine("Error Injecting");
                return;
            }

            D3DFuncLookup d3d9Util;
            try
            {
                d3d9Util = new D3DHookingLibrary.D3DFuncLookup(args[0], args[1]);
            }
            catch (Exception e)
            {
                Console.WriteLine(e.Message);
                Console.WriteLine("Error Initializing D3DHookingLibrary");
                return;
            }
            
            IntPtr dipAddress; 
            IntPtr setStreamSourceAddress;

            try
            {
                dipAddress = (IntPtr)d3d9Util.GetD3DFunction(D3DFuncLookup.D3D9Functions.DrawIndexedPrimitive);
                setStreamSourceAddress = (IntPtr)d3d9Util.GetD3DFunction(D3DFuncLookup.D3D9Functions.SetStreamSource);
                mes.dip_address = dipAddress;
                mes.setStreamSource_address = setStreamSourceAddress;
                injector.getSyringe().CallExport(dllName, "InstallHook", mes);
            }
            catch (Exception e)
            {
                Console.WriteLine(e.Message);
                return;
            }
            while (true) Thread.Sleep(300);
        }

    }
}
