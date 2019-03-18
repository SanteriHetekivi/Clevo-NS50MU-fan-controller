Pour installer, simplement faire :

sudo cp /home/francois/Documents/scripts/clevo-fan/clevo-fan.service /etc/systemd/system/
sudo systemctl enable clevo-fan.service

Vérifier le bon fonctionnement:
systemctl status clevo-fan.service
