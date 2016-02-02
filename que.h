/* this is a circular buffer implementation of queue. You can see about it here
 *  * http://www.csanimated.com/animation.php?t=Circular_buffer */


/* Note: one slot must be left unused for this implementation */
#define QUE_MAX 10

typedef struct Buffer_struct {
    char string[BUFSIZ];
} Buffer;


#define ELE Buffer



void add_match();
void report_matches(char *pat);
int que_init();
void que_error(char *msg);
int que_is_full();
int que_is_empty();
void que_enq(ELE v);
ELE que_deq();

