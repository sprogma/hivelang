using app1.Client.Pages;
using app1.Components;
using Microsoft.AspNetCore.SignalR;
using Microsoft.AspNetCore.ResponseCompression;
using BlazorSignalRApp.Hubs;

namespace app1
{
    public class Program
    {
        public static void Main(string[] args)
        {
            /* prepare code v1 */
            string programJson = File.ReadAllText("../../a.json");
            CodeConverter cvt = new();
            var code = cvt.BuildCode(programJson);

            var builder = WebApplication.CreateBuilder(args);

            // Add services to the container.
            builder.Services.AddRazorComponents()
                .AddInteractiveServerComponents()
                .AddInteractiveWebAssemblyComponents();

            builder.Services.AddHttpClient();

            /* add signals */
            builder.Services.AddSignalR();

            /* enable DI */
            builder.Services.AddSingleton<ServerHive>(sp =>
            {
                var hub = sp.GetRequiredService<IHubContext<HiveHub>>();
                return new ServerHive(hub, code);
            });

            /* run service */
            builder.Services.AddHostedService(sp => sp.GetRequiredService<ServerHive>());

            builder.Services.AddResponseCompression(opts =>
            {
                opts.MimeTypes = ResponseCompressionDefaults.MimeTypes.Concat(
                    ["application/octet-stream"]);
            });

            var app = builder.Build();

            /* run hub */
            app.MapHub<HiveHub>("/hivehub");

            // Configure the HTTP request pipeline.
            if (app.Environment.IsDevelopment())
            {
                app.UseWebAssemblyDebugging();
            }
            else
            {
                app.UseExceptionHandler("/Error");
                // The default HSTS value is 30 days. You may want to change this for production scenarios, see https://aka.ms/aspnetcore-hsts.
                app.UseHsts();
            }

            app.UseStatusCodePagesWithReExecute("/not-found", createScopeForStatusCodePages: true);
            app.UseHttpsRedirection();

            app.UseAntiforgery();
            app.UseResponseCompression();

            app.MapStaticAssets();
            app.MapRazorComponents<App>()
                .AddInteractiveServerRenderMode()
                .AddInteractiveWebAssemblyRenderMode()
                .AddAdditionalAssemblies(typeof(Client._Imports).Assembly);

            app.Run();
        }
    }
}
