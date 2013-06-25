using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.Remoting;
using EasyHook;

namespace D3DHookingLibrary
{
    public static class Hooking
    {
        public static void hook(int pid, string descriptor, string hookExe, string hookDLL32, string hookDLL64, params object[] parameters)
        {
            try
            {
                
                Config.Register(descriptor, hookDLL32, hookExe);

                if(!hookDLL32.ToLower().Equals(hookDLL64.ToLower()))
                    Config.Register(descriptor, hookDLL32, hookExe);

                Console.WriteLine("Registered!");

                EasyHook.RemoteHooking.Inject(pid, hookDLL32, hookDLL64, parameters);

                Console.ReadLine();
            }
            catch (Exception ExtInfo)
            {
                Console.WriteLine("There was an error while connecting to target:\r\n{0}", ExtInfo.ToString());
            }
        }
    }
}
