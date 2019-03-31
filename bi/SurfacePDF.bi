hpp{{
#include <cairo/cairo-pdf.h>
}}

/**
 * A PDF surface. This extends the usual Cairo interface.
 */
class SurfacePDF < Surface {

}

function createPDF(filename:String, widthInPoints:Real, heightInPoints:Real)
    -> Surface {
  mkdir(filename);
  surface:SurfacePDF;
  cpp{{
  surface->surface = cairo_pdf_surface_create(filename.c_str(),
      widthInPoints, heightInPoints);
  }}
  return surface;
}