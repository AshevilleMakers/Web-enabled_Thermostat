-- MySQL dump 10.13  Distrib 5.5.44, for debian-linux-gnu (armv7l)
--
-- Host: localhost    Database: tstat_database
-- ------------------------------------------------------
-- Server version	5.5.44-0+deb7u1

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
-- Table structure for table `thermostat_programs`
--

DROP TABLE IF EXISTS `thermostat_programs`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `thermostat_programs` (
  `userID` int(11) NOT NULL,
  `port` int(11) NOT NULL,
  `start_date` date NOT NULL,
  `stop_date` date NOT NULL,
  `heat_cool` char(1) NOT NULL,
  `weekdays` varchar(13) NOT NULL,
  `times` varchar(630) NOT NULL,
  `temps` varchar(280) NOT NULL,
  `saved` datetime NOT NULL,
  PRIMARY KEY (`userID`,`port`,`start_date`,`stop_date`,`heat_cool`,`saved`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `thermostat_status`
--

DROP TABLE IF EXISTS `thermostat_status`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `thermostat_status` (
  `userID` int(11) NOT NULL,
  `port` int(11) NOT NULL,
  `saved` datetime NOT NULL,
  `system` varchar(4) DEFAULT NULL COMMENT 'possible values are 0 (Off), 1 (On), and A (Auto)',
  `mode` tinyint(4) DEFAULT NULL COMMENT 'possible values are 0 (cooling), 1 (heat pump only), 2 (resistance heat only), 3 (heat pump and resistance heat)',
  `fan` tinyint(4) DEFAULT NULL COMMENT 'possible values are 0 (Off) and 1 (Auto)',
  `temp` float DEFAULT NULL,
  `override` char(1) DEFAULT NULL COMMENT 'Values are NULL (no temperature override), T (temporary - override until next programmed change), H (hold - override indefinitely)',
  PRIMARY KEY (`userID`,`port`,`saved`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `thermostats`
--

DROP TABLE IF EXISTS `thermostats`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `thermostats` (
  `user_id` int(11) NOT NULL,
  `node` int(11) NOT NULL,
  `name` varchar(45) NOT NULL,
  PRIMARY KEY (`user_id`,`node`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `users`
--

DROP TABLE IF EXISTS `users`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `users` (
  `id` mediumint(8) unsigned NOT NULL,
  `password` varchar(32) NOT NULL COMMENT 'This is the customer password encryoted using md5 and salting.',
  `username` varchar(32) NOT NULL,
  PRIMARY KEY (`username`,`id`),
  UNIQUE KEY `username_UNIQUE` (`username`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2016-02-23 14:00:58
