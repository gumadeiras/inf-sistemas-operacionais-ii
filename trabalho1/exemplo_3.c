bplist00�_WebMainResource�	
_WebResourceData_WebResourceMIMEType_WebResourceTextEncodingName^WebResourceURL_WebResourceFrameNameO�<html><head></head><body><pre style="word-wrap: break-word; white-space: pre-wrap;">#include &lt;stdio.h&gt;
#include &lt;stdlib.h&gt;
#include &lt;pthread.h&gt;

void *test_routine (void *arg) {
    double *value;
    value = (double*)malloc(sizeof(double));
    *value = 100.0;
    /*** fix me ***/
}

int main( int argc, char **argv) {
    pthread_t thr;
    double *res;
    int status;

    status = pthread_create( /*** fix me ***/ );
    if (status != 0)
        exit(1);

    status = pthread_join ( /*** fix me ***/ );
    if (status != 0)
        exit(1);

    printf("resultado retornado: %.2f\n", *res);
    free(res);
}
</pre></body></html>Ztext/plainUUTF-8_Shttps://moodle.inf.ufrgs.br/pluginfile.php/65823/mod_resource/content/2/exemplo_3.cP    ( : P n } �%06�                           �