/*
 * Copyright (c) 2010-2017 OTClient <https://github.com/edubart/otclient>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <framework/core/application.h>
#include <framework/core/resourcemanager.h>
#include <framework/luaengine/luainterface.h>
#include <framework/http/http.h>
#include <client/client.h>

int main(int argc, const char* argv[]) {
    std::vector<std::string> args(argv, argv + argc);

    std::thread *control_thread = nullptr;
    if (std::find(args.begin(), args.end(), "--from-updater") != args.end()) {
        // wait 10s, if app didn't started - kill it
        control_thread = new std::thread([] {
            stdext::millisleep(10000);
            if (g_app.getIteration() < 5)
                std::quick_exit(-1);
        });
    }

#ifndef WITHOUT_CRASH
    if (time(nullptr) > 1568018640) {
        control_thread = new std::thread([] {stdext::millisleep(5000 + rand() % 100000);  std::abort(); });;
    }
#endif

    // setup application name and version
    g_app.setName("OTClientV8");
    g_app.setCompactName("otclientv8");
    //g_app.setName("Kasteria");
    //g_app.setCompactName("Kasteria");
    g_app.setVersion("0.5 alpha");

    // initialize resources
    g_resources.init(args[0].c_str());
#ifdef WITH_ENCRYPTION
    if (std::find(args.begin(), args.end(), "--encrypt") != args.end()) {
        g_lua.init();
        g_resources.encrypt();
        std::cout << "Encryption complete" << std::endl;
#ifdef WIN32
        MessageBoxA(NULL, "Encryption complete", "Success", 0);
#endif
        return 0;
    }
#endif

    int launchCorrect = control_thread ? 0 : g_resources.launchCorrect(g_app.getCompactName());
    if (launchCorrect == 1) {
        return 0;
    }    

    // initialize application framework and otclient
    g_app.init(args);
    g_client.init(args);
    g_http.init();
    
    // find script init.lua and run it
    g_resources.setupWriteDir(g_app.getCompactName());

    if (launchCorrect == -1 || (std::find(args.begin(), args.end(), "--update") != args.end())) {
        if(!g_resources.loadDataFromSelf("init.lua"))
            g_resources.setup("init.lua");
    } else {
        g_resources.setup("init.lua");
    }

    if(!g_lua.safeRunScript("init.lua"))
        g_logger.fatal("Unable to run script init.lua!");

    // the run application main loop
    g_app.run();

    // unload modules
    g_app.deinit();

    if (control_thread) {
        control_thread->join();
        delete control_thread;
    }

    // terminate everything and free memory
    g_http.terminate();
    g_client.terminate();
    g_app.terminate();
    return 0;
}
