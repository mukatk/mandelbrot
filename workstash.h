struct work_param {
	int x;
	int y;
	int color;
};

struct node {
	struct work_param value;
	struct node *next;
};

struct node *top = NULL;

void push_work(struct work_param result) {
	struct node *nptr = malloc(sizeof(struct node));
    nptr->value = result;
    nptr->next = top;
    top = nptr;
}

struct work_param pop_work() {
	if (top == NULL)
    {
        printf("\n\nStack is empty ");
    }
    else
    {
        struct node *temp;
        temp = top;
        top = top->next;
		return temp->value;
    }
}
