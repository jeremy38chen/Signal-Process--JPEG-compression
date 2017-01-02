#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>


#define PI 3.14159

typedef struct Pix_RGB
{
    int R;
    int G;
    int B;
} PixRGB;

typedef struct node
{
    struct node *left;
    struct node *right;
    int num;
    char *code;
    short leaf;
    int freq;
} Huffnode;
Huffnode *huff[600];
FILE  *fout,*fin;
int quo =1;


void quicksort(Huffnode *[],int,int);
int partition(Huffnode *[], int, int, Huffnode*);
void swap(Huffnode *[], int, int);
void get_and_init();
void build_tree(Huffnode *[], int);
void encode(Huffnode*, const char*);
void decode(Huffnode *root,PixRGB **PixTns,int H,int W);

PixRGB** DCT_Process(PixRGB **Pixarea );
PixRGB** IDCT_Process(PixRGB **Pixarea);
PixRGB** malloc_2D(int x, int y);
void Quantize(PixRGB **Pixarea2);
void Dequantize(PixRGB **Pixarea);
void ZigZag(PixRGB **Pix2,PixRGB *Zz);

int main(){
    
    int H,W;
    int x, y;
    int m, n,cat;
    int i,j;
    FILE          *fp_s = NULL;    // source file handler
    FILE          *fp_t = NULL;    // target file handler
    char input[100],output[100];
    unsigned int  width, height;   // image width, image height
    unsigned char *image_s = NULL; // source image array
    unsigned char *image_t = NULL; // target image array
    unsigned char R, G, B;         // color of R, G, B
    unsigned int y_avg;            // average of y axle
    unsigned int y_t;              // target of y axle
    unsigned int file_size;           // file size
    unsigned int rgb_raw_data_offset; // RGB raw data offset
    unsigned char header[54] = {
      0x42,        // identity : B
      0x4d,        // identity : M
      0, 0, 0, 0,  // file size
      0, 0,        // reserved1
      0, 0,        // reserved2
      54, 0, 0, 0, // RGB data offset
      40, 0, 0, 0, // struct BITMAPINFOHEADER size
      0, 0, 0, 0,  // bmp width
      0, 0, 0, 0,  // bmp height
      1, 0,        // planes
      24, 0,       // bit per pixel
      0, 0, 0, 0,  // compression
      0, 0, 0, 0,  // data size
      0, 0, 0, 0,  // h resolution
      0, 0, 0, 0,  // v resolution
      0, 0, 0, 0,  // used colors
      0, 0, 0, 0   // important colors
    };

    
    printf("File name:");
    gets(input);
    
    printf("\nOutput File name:");
    gets(output);
 
    fp_s = fopen(input,"rb");
    if (fp_s == NULL) {
      printf("fopen fp_s error\n");
      return -1;
    }

    fseek(fp_s, 10, SEEK_SET);// move offset to 10 to find rgb raw data offset
    fread(&rgb_raw_data_offset, sizeof(unsigned int), 1, fp_s);
    fseek(fp_s, 18, SEEK_SET);// move offset to 18 to get width & height;
    fread(&width,  sizeof(unsigned int), 1, fp_s);
    fread(&height, sizeof(unsigned int), 1, fp_s);
    fseek(fp_s, rgb_raw_data_offset, SEEK_SET);// move offset to rgb_raw_data_offset to get RGB raw data

    image_s = (unsigned char *)malloc((size_t)width * height * 3);
    if (image_s == NULL) {
      printf("malloc images_s error\n");
      return -1;
    }

    image_t = (unsigned char *)malloc((size_t)width * height * 3);
    if (image_t == NULL) {
      printf("malloc image_t error\n");
      return -1;
    }

    fread(image_s, sizeof(unsigned char), (size_t)(long)width * height * 3, fp_s);
    H = height;
    W = width;
    PixRGB **Pix2;

    PixRGB **PixALL=malloc_2D(height,width);
    PixRGB **Pixarea=malloc_2D(8,8);
    PixRGB *Zz=(PixRGB*)malloc(sizeof(PixRGB)*64);
    PixRGB *Rl=(PixRGB*)malloc(sizeof(PixRGB)*64);
   
    // Read value of RGB
   for(m = 0; m != height; ++m) {
      for(n = 0; n != width; ++n) {
        R = *(image_s + 3 * (width * m + n) + 2);
        G = *(image_s + 3 * (width * m + n) + 1);
        B = *(image_s + 3 * (width * m + n) + 0);

        PixALL[m][n].R = R;
        PixALL[m][n].G = G;
        PixALL[m][n].B = B;


      }
   }
    // Divide into 8x8 matrix
    for(x=0; x<height; x+=8)
    {
        for(y=0; y<width; y+=8)
        {
            for(m=0; m<8; m++)
            {
                for(n=0; n<8; n++)
                {
                    if((x+m>=height)||(y+n>=width))
                    {
                        Pixarea[m][n].R=0;
                        Pixarea[m][n].G=0;
                        Pixarea[m][n].B=0;
                    }
                    else
                    {
                        Pixarea[m][n].R = PixALL[x+m][y+n].R -128;

                        Pixarea[m][n].G = PixALL[x+m][y+n].G -128;
           
                        Pixarea[m][n].B = PixALL[x+m][y+n].B -128;
  
                    }
                }

            }
            // Do DCT
            Pix2=DCT_Process(Pixarea);
            // Do quantization
            Quantize(Pix2);
            // Do zigzag
            ZigZag(Pix2,Zz);
            for(m=0; m<8; m++)
            {
                for(n=0; n<8; n++)
                {

                    if((x+m<height)&&(y+n<width))
                    {
                        PixALL[x+m][y+n].R=Pix2[m][n].R;
                
                        PixALL[x+m][y+n].G=Pix2[m][n].G;
            
                        PixALL[x+m][y+n].B=Pix2[m][n].B;


                    }

                }
            }
        }
    }

    // Built Huffman Tree
    int *no = (int*)calloc(sizeof(int),600);
    for(x=0; x<height; x++)
    {
        for(y=0; y<width; y++)
        {
            i=PixALL[x][y].R+300;
            no[i]+=1;
            i=PixALL[x][y].G+300;
            no[i]+=1;
            i=PixALL[x][y].B+300;
            no[i]+=1;
        }
    }
    for(j=0,i=0; j<600; j++)
        if(no[j]!=0)
        {
            (huff[i])=(Huffnode*)malloc(sizeof(Huffnode));
            (huff[i])->left=NULL;
            (huff[i])->right=NULL;
            (huff[i])->num=j;
            (huff[i])->freq=no[j];
            (huff[i])->leaf=1;

            i++;

        }
    quicksort(huff, 0, i);
    while(--i>0)
    {
        build_tree(huff, i);
        quicksort(huff, 0, i);
    }
    fout = fopen("Huff_codebook.txt","w");
    huff[0]->code=(char*)malloc(sizeof(char));
    (huff[0]->code)[0]='\0';

    encode(huff[0], huff[0]->code);
    fclose(fout);

   // Encode
   fout=fopen("Huff_encode.txt","wb");
    for(x=0; x<height; x++)
        for(y=0; y<width; y++)
        {
            cat=PixALL[x][y].R+300;
            for(n=1; n<quo; n++)
                if(cat==huff[n]->num)
                {
                    fprintf(fout, "%s",huff[n]->code);
                    break;
                }
            cat=PixALL[x][y].G+300;
            for(n=1; n<quo; n++)
                if(cat==huff[n]->num)
                {
                    fprintf(fout, "%s",huff[n]->code);
                    break;
                }
            cat=PixALL[x][y].B+300;
            for(n=1; n<quo; n++)
                if(cat==huff[n]->num)
                {
                    fprintf(fout, "%s",huff[n]->code);
                    break;
                }

        }
    fclose(fout);
    fclose(fin);
   
    
    
    // Decode
    fin = fopen("Huff_encode.txt","r+");
    decode(huff[0],PixALL,height,width);

    for(x=0; x<height; x+=8)
    {
        for(y=0; y<width; y+=8)
        {
            for(m=0; m<8; m++)
            {
                for(n=0; n<8; n++)
                {
                    if((x+m>=height)||(y+n>=width))
                    {
                        Pixarea[m][n].R=0;
                        Pixarea[m][n].G=0;
                        Pixarea[m][n].B=0;
                    }
                    else
                    {
                        Pixarea[m][n].R = PixALL[x+m][y+n].R;
                        Pixarea[m][n].G = PixALL[x+m][y+n].G;
                        Pixarea[m][n].B = PixALL[x+m][y+n].B;
                    }
                }

            }
            Dequantize(Pixarea);
            Pix2=IDCT_Process(Pixarea);

            for(m=0; m<8; m++)
            {
                for(n=0; n<8; n++)
                {

                    if((x+m<height)&&(y+n<width))
                    {
                        PixALL[x+m][y+n].R=Pix2[m][n].R +128;
                        PixALL[x+m][y+n].G=Pix2[m][n].G +128;
                        PixALL[x+m][y+n].B=Pix2[m][n].B +128;
                       
                    }

                }
            }
        }
    }




    // Write to new bmp
    fp_t = fopen(output, "wb");
    if (fp_t == NULL) {
      printf("fopen fname_t error\n");
        return -1;
      }

     // file size
     file_size = width * height * 3 + rgb_raw_data_offset;
     header[2] = (unsigned char)(file_size & 0x000000ff);
     header[3] = (file_size >> 8)  & 0x000000ff;
     header[4] = (file_size >> 16) & 0x000000ff;
     header[5] = (file_size >> 24) & 0x000000ff;

     // width
     header[18] = width & 0x000000ff;
     header[19] = (width >> 8)  & 0x000000ff;
     header[20] = (width >> 16) & 0x000000ff;
     header[21] = (width >> 24) & 0x000000ff;

     // height
     header[22] = height &0x000000ff;
     header[23] = (height >> 8)  & 0x000000ff;
     header[24] = (height >> 16) & 0x000000ff;
     header[25] = (height >> 24) & 0x000000ff;

   // Write header
   fwrite(header, sizeof(unsigned char), rgb_raw_data_offset, fp_t);
   // Write image
   for (x=0; x<height; x++)
    {
        for(y=0; y<width; y++)
        {
            
            fwrite(&PixALL[x][y].R, sizeof(char),1,fp_t);
            fwrite(&PixALL[x][y].G, sizeof(char),1,fp_t);
            fwrite(&PixALL[x][y].B, sizeof(char),1,fp_t);
        }
    }

     fclose(fp_s);
     fclose(fp_t);
     fclose(fin);



    free(Pix2);
    free(PixALL);
    free(Zz);
    free(Rl);
    return 0;
}



PixRGB** DCT_Process(PixRGB **Pixarea )
{

    int x, y, m , n;
    float temp_B, temp_G, temp_R, rad_m, rad_n, C_x, C_y;
    PixRGB **Pixarea2;
    Pixarea2 = (PixRGB **) malloc(sizeof(PixRGB*)*8);
    for(x=0; x<8; x++)
    {
        Pixarea2[x] = (PixRGB *) malloc(sizeof(PixRGB*)*8*8);
    }

    for(x=0 ; x<8 ; x++ )
    {
        for(y=0 ; y<8 ; y++)
        {
            Pixarea2[x][y].R = 0;
            Pixarea2[x][y].G = 0;
            Pixarea2[x][y].B = 0;
            for(m=0 ; m<8 ; m++ )
            {
                for(n=0 ; n<8 ; n++ )
                {
                    C_x = x==0? (1/sqrt(2)) : 1;
                    C_y = y==0? (1/sqrt(2)) : 1;
                    rad_m = (2*m+1)*x*PI/16;
                    rad_n = (2*n+1)*y*PI/16;
                    temp_R = (2*C_x*C_y/8) * Pixarea[m][n].R * cos(rad_m) * cos(rad_n);
                    temp_G = (2*C_x*C_y/8) * Pixarea[m][n].G * cos(rad_m) * cos(rad_n);
                    temp_B = (2*C_x*C_y/8) * Pixarea[m][n].B * cos(rad_m) * cos(rad_n);
                    Pixarea2[x][y].R += temp_R;
                    Pixarea2[x][y].G += temp_G;
                    Pixarea2[x][y].B += temp_B;
                }
            }
        }
       
    }
    return Pixarea2;
}
PixRGB** IDCT_Process(PixRGB **Pixarea)
{
int x, y, m , n;
    float temp_B, temp_G, temp_R, rad_m, rad_n, C_m, C_n;
    PixRGB **Pixarea2;
    Pixarea2 = (PixRGB **) malloc(sizeof(PixRGB*)*8);
    for(x=0; x<8; x++)
    {
        Pixarea2[x] = (PixRGB *) malloc(sizeof(PixRGB*)*8*8);
    }
    for(x=0 ; x<8 ; x++ )
    {
        for(y=0 ; y<8 ; y++)
        {
            Pixarea2[x][y].R = 0;
            Pixarea2[x][y].G = 0;
            Pixarea2[x][y].B = 0;
            for(m=0 ; m<8 ; m++ )
            {
                for(n=0 ; n<8 ; n++ )
                {

                    C_m = m==0? (1/sqrt(2)) : 1;
                    C_n = n==0? (1/sqrt(2)) : 1;
                    rad_m = (2*x+1)*m*PI/16;
                    rad_n = (2*y+1)*n*PI/16;
                    temp_R = (2*C_m*C_n/8) * Pixarea[m][n].R * cos(rad_m) * cos(rad_n);
                    temp_G = (2*C_m*C_n/8) * Pixarea[m][n].G * cos(rad_m) * cos(rad_n);
                    temp_B = (2*C_m*C_n/8) * Pixarea[m][n].B * cos(rad_m) * cos(rad_n);
                    Pixarea2[x][y].R += temp_R;
                    Pixarea2[x][y].G += temp_G;
                    Pixarea2[x][y].B += temp_B;
                }
            }
        }

    }
    return Pixarea2;

}
void Quantize(PixRGB **Pix2)
{
    int i,j;
    int Quant[8][8]= {
        {8,6,6,7,6,5,8,7},
        {7,7,9,9,8,10,12,20},
        {13,12,11,11,12,25,18,19},
        {15,20,29,26,31,30,29,26},
        {28,28,32,36,46,39,32,34},
        {44,35,28,28,40,55,41,44},
        {48,49,52,52,52,31,39,57},
        {61,56,50,60,46,51,52,50}
    };// Quantization table


    for(i=0; i<8; i++)
        for(j=0; j<8; j++)
        {
            Pix2[i][j].R=(Pix2[i][j].R/Quant[i][j]);
            Pix2[i][j].G=(Pix2[i][j].G/Quant[i][j]);
            Pix2[i][j].B=(Pix2[i][j].B/Quant[i][j]);
        }
    return;

}
void Dequantize(PixRGB **Pixarea)
{
    int i,j;
    int Quant[8][8]= {
        {8,6,6,7,6,5,8,7},
        {7,7,9,9,8,10,12,20},
        {13,12,11,11,12,25,18,19},
        {15,20,29,26,31,30,29,26},
        {28,28,32,36,46,39,32,34},
        {44,35,28,28,40,55,41,44},
        {48,49,52,52,52,31,39,57},
        {61,56,50,60,46,51,52,50}
    };// Quantization table


    for(i=0; i<8; i++)
        for(j=0; j<8; j++)
        {
            Pixarea[i][j].R=((Pixarea[i][j].R)*Quant[i][j]);
            Pixarea[i][j].G=((Pixarea[i][j].G)*Quant[i][j]);
            Pixarea[i][j].B=((Pixarea[i][j].B)*Quant[i][j]);
        }
    return;
}

PixRGB** malloc_2D(int x, int y)
{
    PixRGB **Array, *Data;
    int i;
    Array = (PixRGB**)malloc(x*sizeof(PixRGB *));
    Data = (PixRGB*)malloc(x*y*sizeof(PixRGB));
    for(i = 0; i < x; i++, Data += y)
        Array[i] = Data;
    return Array;
}

void ZigZag(PixRGB **Pix2,PixRGB *Zz)
{
    int i=0,j=0,k=0,d=0;
    while(k<36)
    {
    
        Zz[k++] = Pix2[i][j];
        if((i==0)&&(j%2==0))
        {
            j++;
            d=1;
        }
        else if((j==0)&&(i%2==1))
        {
            i++;
            d=0;
        }
        else if(d==0)
        {
            i--;
            j++;
        }
        else
        {
            i++;
            j--;
        }
    }
    i = 7;
    j = 1;
    d = 0;
    while(k<64)
    {
        
        Zz[k++] = Pix2[i][j];
        if((i==7)&&(j%2==0))       
        {
            j++;
            d=0;
        }
        else if((j==7)&&(i%2==1))
        {
            i++;
            d=1;
        }
        else if(d==0)
        {
            i--;
            j++;
        }
        else
        {
            i++;
            j--;
        }
    }
}

void build_tree(Huffnode *root[], int n)
{
    Huffnode *ptr = (Huffnode*)malloc(sizeof(Huffnode));
    ptr->left=(root[n]);
    ptr->right=(root[n-1]);
    ptr->leaf=0;
    ptr->freq=ptr->left->freq+ptr->right->freq;
    (root[n-1])=ptr;
}
void encode(Huffnode *root, const char *ecode)
{
    char *lcode= NULL,*rcode = NULL;
    if(root == NULL)
        return;
    if(root->leaf)
    {
        root->code=(char*)malloc(sizeof(char)*(strlen(ecode)+1));
        strcpy(root->code, ecode);
        if(root->num == '\n')
            fprintf(fout,"Symbol %s%s","\\n"," code:");
        else
            fprintf(fout,"Symbol %d%s", (root->num)-300," code:");
        fprintf(fout, "%s\n", root->code);
        huff[quo++]=root;
    }
    lcode = (char*)malloc(sizeof(char)*(strlen(ecode)+2));
    strcpy(lcode, ecode);
    strcat(lcode, "0");
    encode(root->left,lcode);
    free(lcode);
    rcode = (char*)malloc(sizeof(char)*(strlen(ecode)+2));
    strcpy(rcode, ecode);
    strcat(rcode,"1");
    encode(root->right, rcode);
    free(rcode);
}


int partition(Huffnode *root[], int low, int high, Huffnode *pivot)
{
    do
    {
        while((root[++low])->freq>pivot->freq);
        while(high!=0 && (root[--high])->freq<pivot->freq);
        swap(root, low, high);
    }
    while(low<high);
    swap(root, low, high);
    return low;
}
void quicksort(Huffnode *root[], int left, int right)
{

    int index;
    if(left>=right-1)
        return;
    index = partition(root, left-1,right-1,root[right-1]);
    swap(root, index, right-1);
    quicksort(root, left, index);
    quicksort(root, index+1, right);
}
void swap(Huffnode *root[],int low, int high)
{
    Huffnode *tmp = (root[low]);
    root[low]=root[high];
    root[high]=tmp;
}

void decode(Huffnode *root,PixRGB **PixALL,int H,int W)
{
    char c;
    int count=0;
    int x,y;
    while(!feof(fin))
    {
        for(x=0; x<H; x++)
            for(y=0; y<W; y++)
            {
                count+=1;
                if(root->leaf)
                {
                    if(count==1)
                    {
                        PixALL[x][y].R=PixALL[x][y].R;
                    }
                    else if(count==2)
                    {
                        PixALL[x][y].G=PixALL[x][y].G;
                    }
                    else
                    {
                        PixALL[x][y].B=PixALL[x][y].B;
                        count=0;
                    }
                    root=huff[0];
                }
                else
                {
                    c=fgetc(fin);
                    if(c == '0')
                        root=root->left;
                    else if (c == '1')
                        root=root->right;
                }
            }
    }
}

