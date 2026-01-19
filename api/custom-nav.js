/* custom-nav.js - English names for functions and variables */
document.addEventListener("DOMContentLoaded", function() {
    
    // 1. INYECTAR LA BARRA DE "VOLVER"
    // Creamos el div
    var banner = document.createElement("div");
    banner.style.cssText = "padding: 10px 15px; background: #37474f; color: white; text-align: right; border-bottom: 1px solid #263238; font-family: Roboto, sans-serif;";
    
    // El enlace (Usamos innerHTML para facilitar la tilde con código HTML)
    banner.innerHTML = '<a href="../index.html" style="color: #ffccbc; font-weight: bold; text-decoration: none;">' +
                       '<i class="fa fa-arrow-left"></i> Volver a la Documentaci&oacute;n' + 
                       '</a>';
    
    // Lo insertamos al principio del body
    document.body.insertBefore(banner, document.body.firstChild);

    // 2. ARREGLAR FAVICON (Si no carga por defecto)
    var link = document.querySelector("link[rel~='icon']");
    if (!link) {
        link = document.createElement('link');
        link.rel = 'icon';
        document.getElementsByTagName('head')[0].appendChild(link);
    }
    // Aseguramos que apunte al favicon que está en la raíz de api/
    // Doxygen suele copiarlo ahí si estaba en HTML_EXTRA_FILES
    link.href = 'favicon.ico';

    // 3. CAMBIAR EL TÍTULO DE LA PESTAÑA (Opcional, estética)    
    document.title = "API Reference - " + document.title;
});