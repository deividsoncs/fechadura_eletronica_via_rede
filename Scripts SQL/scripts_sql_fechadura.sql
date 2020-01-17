CREATE DATABASE  IF NOT EXISTS `arduino_db` /*!40100 DEFAULT CHARACTER SET latin1 */;
USE `arduino_db`;
-- MySQL dump 10.13  Distrib 5.6.17, for Win64 (x86_64)
--
-- Host: 10.0.10.203    Database: arduino_db
-- ------------------------------------------------------
-- Server version	5.5.46-0ubuntu0.12.04.2

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `historico`
--

DROP TABLE IF EXISTS `historico`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `historico` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `id_usuario` int(11) DEFAULT NULL,
  `data_hora` datetime DEFAULT NULL,
  `porta` varchar(20) DEFAULT NULL,
  `tipo_acesso` varchar(4) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=197 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `usuarios`
--

DROP TABLE IF EXISTS `usuarios`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `usuarios` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `nome` varchar(60) DEFAULT NULL,
  `chave` varchar(100) DEFAULT NULL,
  `id_sipom` int(11) DEFAULT NULL,
  `senha` varchar(50) DEFAULT NULL,
  `rg` varchar(10) DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `chave_UNIQUE` (`chave`)
) ENGINE=InnoDB AUTO_INCREMENT=3 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping events for database 'arduino_db'
--

--
-- Dumping routines for database 'arduino_db'
--
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2016-07-01 11:56:49


SELECT * FROM arduino_db.usuarios;
SELECT * FROM sipom.duino_usuarios;

#Importação
set FOREIGN_KEY_CHECKS=1;
INSERT INTO sipom.duino_usuarios (chave, nome, rg, senha, id_sipom) SELECT chave, nome, rg, senha, id_sipom FROM arduino_db.usuarios;

SELECT chave, nome, rg, senha, id_sipom FROM arduino_db.usuarios;

select * from arduino_db.historico ORDER BY (id) DESC;

INSERT INTO arduino_db.historico (porta, data_hora) VALUES ('porta  de baixo', now());
INSERT INTO arduino_db.historico (porta, data_hora, id_usuario, tipo_acesso) VALUES ('%s', now(), 123, 'RG');

SELECT id, nome FROM arduino_db.usuarios WHERE(chave = 123456);

SELECT SUBSTRING(nome,1,16), id FROM arduino_db.usuarios;

#limpar tabela histórico
truncate table historico;

#Pesquisa de usuários por RG e DATA
SELECT nome, data_hora, tipo_acesso 
	FROM usuarios u 
	JOIN historico h ON(u.id = h.id_usuario) 
	WHERE u.rg LIKE '207415' AND data_hora BETWEEN '2016-06-23 16:11:00' AND now();


