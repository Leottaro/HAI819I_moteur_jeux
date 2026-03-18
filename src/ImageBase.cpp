/******************************************************************************
 * ICAR_Library
 *
 * Fichier : ImageBase.cpp
 *
 * Description : Voir le fichier .h
 *
 * Auteur : Mickael Pinto
 *
 * Mail : mickael.pinto@live.fr
 *
 * Date : Octobre 2012
 *
 *******************************************************************************/

#include <GL/glew.h>
#include "ImageBase.h"
#include "image_ppm.h"
#include <vector>

ImageBase::ImageBase(void) {
    isValid = false;
    init();
}

ImageBase::ImageBase(int imWidth, int imHeight, bool isColor) {
    isValid = false;
    init();

    color = isColor;
    height = imHeight;
    width = imWidth;
    nTaille = height * width * (color ? 3 : 1);

    if (nTaille == 0)
        return;

    allocation_tableau(data, OCTET, nTaille);
    dataD = (double *)malloc(sizeof(double) * nTaille);
    isValid = true;
}

ImageBase::ImageBase(const char *filename) {
    isValid = false;
    init();
    load(filename);
}

ImageBase::~ImageBase(void) {
    reset();
}

void ImageBase::init() {
    if (isValid) {
        free(data);
        free(dataD);
    }

    data = 0;
    dataD = 0;
    height = width = nTaille = 0;
    isValid = false;
}

void ImageBase::reset() {
    if (isValid) {
        free(data);
        free(dataD);
    }
    isValid = false;
    clearShaderData();
}

void ImageBase::load(const char *filename) {
    init();

    int l = strlen(filename);

    if (l <= 4) // Le fichier ne peut pas etre que ".pgm" ou ".ppm"
    {
        printf("Chargement de l'image impossible : Le nom de fichier n'est pas conforme, il doit comporter l'extension, et celle ci ne peut �tre que '.pgm' ou '.ppm'");
        exit(0);
    }

    int nbPixel = 0;

    if (strcmp(filename + l - 3, "pgm") == 0) // L'image est en niveau de gris
    {
        color = false;
        lire_nb_lignes_colonnes_image_pgm(const_cast<char *>(filename), &height, &width);
        nbPixel = height * width;

        nTaille = nbPixel;
        allocation_tableau(data, OCTET, nTaille);
        lire_image_pgm(const_cast<char *>(filename), data, nbPixel);
    } else if (strcmp(filename + l - 3, "ppm") == 0) // L'image est en couleur
    {
        color = true;
        lire_nb_lignes_colonnes_image_ppm(const_cast<char *>(filename), &height, &width);
        nbPixel = height * width;

        nTaille = nbPixel * 3;
        allocation_tableau(data, OCTET, nTaille);
        lire_image_ppm(const_cast<char *>(filename), data, nbPixel);
    } else {
        printf("Chargement de l'image impossible : Le nom de fichier n'est pas conforme, il doit comporter l'extension, et celle ci ne peut �tre que .pgm ou .ppm");
        exit(0);
    }

    dataD = (double *)malloc(sizeof(double) * nTaille);

    isValid = true;
}

bool ImageBase::save(const char *filename) {
    if (!isValid) {
        printf("Sauvegarde de l'image impossible : L'image courante n'est pas valide");
        exit(0);
    }

    if (color)
        ecrire_image_ppm(const_cast<char *>(filename), data, height, width);
    else
        ecrire_image_pgm(const_cast<char *>(filename), data, height, width);

    return true;
}

void ImageBase::initShaderData(GLuint _location) {
    location = _location;

    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0 + location);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    if (getColor()) {
        std::vector<float> image_data(width * height * 4);
        for (int j = 0; j < width * height; j++) {
            const unsigned char *pixel = getPixel(j);
            image_data[j * 4] = float(pixel[0]) / 255.;
            image_data[j * 4 + 1] = float(pixel[1]) / 255.;
            image_data[j * 4 + 2] = float(pixel[2]) / 255.;
            image_data[j * 4 + 3] = 1.;
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, image_data.data());
        glBindImageTexture(location, texture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
    } else {
        std::vector<float> image_data(width * height);
        for (int j = 0; j < width * height; j++) {
            const unsigned char *pixel = getPixel(j);
            image_data[j] = float(pixel[0]) / 255.;
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, image_data.data());
        glBindImageTexture(location, texture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
    }
    glGenerateMipmap(GL_TEXTURE_2D);
}

void ImageBase::clearShaderData() {
    if (texture) {
        glDeleteTextures(1, &texture);
        texture = 0;
    }
}
