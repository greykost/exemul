
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <ctype.h>
#include <math.h>

typedef struct t_order *pOpder;

typedef struct t_order
{
    unsigned int id;        // идентификатор
    unsigned int count;     // количество
    float price;            // цена
    char order;             // ордер/отмена
    char side;              // покупка/продажа
    char rez[2];
    pOpder pnext;           // следующий
    pOpder pprev;           // предыдущий
} TOrder; // заявка

static TOrder *pOrderList  = NULL; // заявки

typedef struct t_trade
{
    unsigned int id;        // идентификатор сделки
    unsigned int id1;       // идентификатор заявки выставленной ранее
    unsigned int id2;       // идентификатор заявки инициатора сделки
    unsigned int count;     // количество
    float price;            // цена
    char trade;             // сделка/отмена
    char side;              // покупка/продажа
    char rez[2];
} TTrade; // сделка

static unsigned int TradeCount = 0;

pOpder OrderAdd(TOrder *pOr);
bool OrderRemove(unsigned int id);
unsigned long OrderCount(char side);
TOrder * FindOrderLast(void);
TOrder * FindOrderID(unsigned int id);

void TradeLog(TTrade *pTr);
void TradeAdd(TOrder *pOr);

int compareSell(const void * pOr1, const void * pOr2);
int compareBuy(const void * pOr1, const void * pOr2);

void ListClear(void);

static FILE *fw = NULL;

int main(int argc, char *argv[])
{
    printf("ExEmul started!\n");

    chdir("./");

    FILE *fo = fopen("input.txt", "r");

    if(!fo)
    {
        printf("Error fopen input.txt !\n");
        return 1;
    }

    fw = fopen("output.txt", "w+");

    if(!fw)
    {
        printf("Error fopen output.txt !\n");
        return 1;
    }

    TOrder ord;
    bzero(&ord, sizeof(TOrder));
    char buf[1024] = { 0 };
    char *pBuf = buf;
    size_t linelen;

    while (getline(&pBuf, &linelen, fo) != EOF)
    {
        if(pBuf[0] == 'O')
        {
            sscanf(buf, "%c,%u,%c,%u,%f", &ord.order, &ord.id, &ord.side, &ord.count, &ord.price);
            OrderAdd(&ord);
            TradeAdd(&ord);
            bzero(buf, sizeof(buf));
            bzero(&ord, sizeof(TOrder));
        }
        else if(pBuf[0] == 'C')
        {
            char c;
            unsigned int id;
            sscanf(buf, "%c,%u", &c, &id);
            bool res = OrderRemove(id);
            // Отмена в лог
            if(res)
            {
                TTrade tr;
                tr.trade = 'X';
                tr.id = id;
                TradeLog(&tr);
            }
        }
    }

    fclose(fo);

    fclose(fw);

    ListClear();

    printf("ExEmul exit!\n");
    return 0;
}


// Новая сделка <+++>
void TradeAdd(TOrder *pOr)
{
    TOrder *last = FindOrderID(pOr->id);

    if (last == NULL)
    {
        return;
    }

    // Тип сделки
    char trside;

    if(pOr->side == 'B')
    {
        trside = 'S';
    }
    else if(pOr->side == 'S')
    {
        trside = 'B';
    }

    const unsigned long cnt = OrderCount(trside);

    TOrder Buf[cnt];
    bzero(&Buf, sizeof(Buf));

    TOrder *pNode = pOrderList;
    unsigned long k = 0;
    while (pNode != NULL)
    {
        if(pNode->side == trside)
        {
            Buf[k] = *pNode;
            k++;
        }
        pNode = pNode->pnext;
    }

    // Сортировка по типу заявки возрастание/убывание
    if(pOr->side == 'B')
    {
        qsort(Buf, cnt, sizeof(TOrder), compareBuy);
    }
    else if(pOr->side == 'S')
    {
        qsort(Buf, cnt, sizeof(TOrder), compareSell);
    }

    // Выполнение сделки
    for(unsigned long i = 0; i < cnt; i++)
    {
        //if (pOr->side == 'B')
        {
            if((pOr->side == 'B') ? (Buf[i].price <= last->price) : (Buf[i].price >= last->price))
            {
                if(Buf[i].count >= last->count) // сделка завершена
                {
                    TradeCount++;
                    TTrade tr;
                    tr.id = TradeCount;
                    tr.side = trside;
                    tr.id1 = Buf[i].id;
                    tr.id2 = last->id;
                    tr.count = last->count;
                    tr.price = Buf[i].price;
                    tr.trade = 'T';
                    //
                    TradeLog(&tr);

                    //
                    if(Buf[i].count > last->count)
                    {
                        TOrder *p = FindOrderID(Buf[i].id);
                        p->count -= last->count;
                    }
                    else
                        OrderRemove(Buf[i].id);

                    //
                    OrderRemove(last->id);

                    break;
                }
                else // сделка частичная
                {
                    TradeCount++;
                    TTrade tr;
                    tr.id = TradeCount;
                    tr.side = trside;
                    tr.id1 = Buf[i].id;
                    tr.id2 = last->id;
                    tr.count = Buf[i].count;
                    tr.price = Buf[i].price;
                    tr.trade = 'T';
                    //
                    TradeLog(&tr);
                    //
                    last->count -= Buf[i].count;
                    //
                    OrderRemove(Buf[i].id);
                }
            }
        }
        /*else if (pOr->side == 'S')
        {
            if(Buf[i].price >= last->price)
            {
                if(Buf[i].count >= last->count) // сделка завершена
                {
                    TradeCount++;
                    TTrade tr;
                    tr.id = TradeCount;
                    tr.side = 'B';
                    tr.id1 = Buf[i].id;
                    tr.id2 = last->id;
                    tr.count = last->count;
                    tr.price = Buf[i].price;
                    tr.trade = 'T';
                    printf("T,%u,%c,%u,%u,%u,%.2f\n", tr.id, tr.side, tr.id1, tr.id2, tr.count, tr.price);
                    TradeLog(&tr);

                    //
                    if(Buf[i].count > last->count)
                    {
                        TOrder *p = FindOrderID(Buf[i].id);
                        p->count -= last->count;
                    }
                    else
                        OrderRemove(Buf[i].id);

                    //
                    OrderRemove(last->id);

                    break;
                }
                else // сделка частичная
                {
                    TradeCount++;
                    TTrade tr;
                    tr.id = TradeCount;
                    tr.side = 'B';
                    tr.id1 = Buf[i].id;
                    tr.id2 = last->id;
                    tr.count = Buf[i].count;
                    tr.price = Buf[i].price;
                    tr.trade = 'T';
                    printf("T,%u,%c,%u,%u,%u,%.2f\n", tr.id, tr.side, tr.id1, tr.id2, tr.count, tr.price);
                    TradeLog(&tr);

                    last->count -= Buf[i].count;

                    OrderRemove(Buf[i].id);
                }
            }
        }*/
    }
}

// Новая заявка <+++>
pOpder OrderAdd(TOrder *pOr)
{
    unsigned long ulSize = sizeof(TOrder);

    pOpder pNew = (pOpder)calloc(ulSize, 1);

    if (pNew == NULL)
    {
        return 0;
    }

    TOrder *pTail = FindOrderLast();

    pNew->pprev = NULL;
    pNew->pnext = NULL;

    if (pTail == NULL)
    {
        pOrderList = pNew;
        pOrderList->pnext = NULL;
        pOrderList->pprev = NULL;
    }
    else
    {
        pTail->pnext = pNew;
        pNew->pprev = pTail;
    }

    pNew->order = pOr->order;
    pNew->id = pOr->id;
    pNew->side = pOr->side;
    pNew->count = pOr->count;
    pNew->price = pOr->price;

    return pNew;
}

// Послендний в списке <+++>
TOrder * FindOrderLast()
{
    if (pOrderList != NULL)
    {
        TOrder *pNode = pOrderList;

        while (pNode != NULL && pNode->pnext != NULL)
            pNode = pNode->pnext;

        return pNode;
    }
    else
        return NULL;
}

// Удаление по идентификатору <+++>
bool OrderRemove(unsigned int id)
{
    TOrder *pprev;
    TOrder *pnext;

    TOrder *pcurrent = FindOrderID(id);

    if (pcurrent == NULL)
        return false;

    pprev = pcurrent->pprev;
    pnext = pcurrent->pnext;

    if (pprev != NULL)
        pprev->pnext = pnext;
    else
    {
        pOrderList = pnext;
    }

    if (pnext != NULL)
        pnext->pprev = pprev;

    free(pcurrent);

    return true;
}

// Чистка <+++>
void ListClear()
{
    unsigned long cnt = 0;

    TOrder * temp = NULL;
    while(pOrderList != NULL)
    {
        temp = pOrderList;
        pOrderList = pOrderList->pnext;
        free(temp);
        cnt++;
    }
}

// Поиск заявки по идентификатору <+++>
TOrder * FindOrderID(unsigned int id)
{
    if (!id)
        return NULL;

    if (pOrderList != NULL)
    {
        TOrder *pNode = pOrderList;

        while (pNode != NULL)
        {
            if (pNode->id == id)
            {
                return pNode;
            }

            pNode = pNode->pnext;
        }
    }
    return NULL;
}

// Количество заявок по типу покупка/продажа <+++>
unsigned long OrderCount(char side)
{
    if (pOrderList != NULL)
    {
        TOrder *pNode = pOrderList;

        unsigned long cnt = 0;

        while (pNode != NULL)
        {
            if (pNode->side == side)
                cnt++;

            pNode = pNode->pnext;
        }
        return cnt;
    }
    else
        return 0;
}

// Продажа: сортировка по цене в сторону убывания <+++>
int compareSell(const void * pOr1, const void * pOr2)
{
    const TOrder * pOr1_ = (const TOrder *)pOr1;
    const TOrder * pOr2_ = (const TOrder *)pOr2;

    int res = (int)(1000000.f * pOr2_->price - 1000000.f * pOr1_->price);

    // При одинаковой цене сортировка по ID в сторону возрастания
    return res == 0 ? (int)(pOr1_->id - pOr2_->id) : res;
}

// Покупка: сортировка по цене в сторону возрастания <+++>
int compareBuy(const void * pOr1, const void * pOr2)
{
    const TOrder * pOr1_ = (const TOrder *)pOr1;
    const TOrder * pOr2_ = (const TOrder *)pOr2;

    int res = (int)(1000000.f * pOr1_->price - 1000000.f * pOr2_->price);

    // При одинаковой цене сортировка по ID в сторону возрастания
    return res == 0 ? (int)(pOr1_->id - pOr2_->id) : res;
}

// Запись лога сделок в файл <+++>
void TradeLog(TTrade *pTr)
{
    if(pTr->trade == 'T')
    {
        fprintf(fw, "T,%u,%c,%u,%u,%u,%.2f\n", pTr->id, pTr->side, pTr->id1, pTr->id2, pTr->count, pTr->price);
    }
    else if(pTr->trade == 'X')
    {
        fprintf(fw, "X,%u\n", pTr->id);
    }
}

